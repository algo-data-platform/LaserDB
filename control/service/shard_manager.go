package service

import (
	"encoding/json"
	"fmt"
	"math"
	"sort"
	"sync"
	"time"

	"github.com/jinzhu/gorm"

	"laser-control/common"
	"laser-control/context"
)

type ShardRole int32

const (
	Leader ShardRole = iota + 1
	Follower
)

type ControlStatus uint32

const (
	Common ControlStatus = iota + 1
	FollowerDynamicAssign
	LeaderDynamicAssign
	LeaderBackup
	LeaderRecover
	FollowerReplace
)

type ShardVersionStatus uint32

const (
	Active ShardVersionStatus = iota + 1
	Default
)

/*
任何变更首先 调用 gettoken 接口获取 唯一 token
如果当前有 token 正在处理 拒绝处理, 每个类型不能相互穿插，只能保证全部完成才可以执行下一次变更

1. 扩缩容从机器：
	a. 重新分配，将原来节点要删除的分片状态置为 FollowerDeleteStatus.
	b. 将新节点的分片状态改为 FollowerCreateStatus
	c. 新节点服务状态置为不可用(调用接口，目的节点上的分片置为不可用状态)
	d. 生成临时配置，入库、 check 服务状态、推上线
		check 服务状态条件：
		  （1）多个 group 中间，有且仅有一个leader 分片，状态 可用；
			（2）每个 group 里面，所有分片至少存在一份可用的；
	e. check 主从同步状态 （没有diff，说明线上切换完成）
	f. FollowerCreateStatus 状态的服务改为可用，FollowerCreateStatus 改为不可用
	g. 将 FollowerDeleteStatus 类型的分片删除掉，重新入库、check 服务状态、推上线
	h. 完成扩容

附加信息：【从机器扩容】记录删除的分片、和添加分片的信息

2. 扩缩容主机器：

	a. 重新分配，将原来节点要删除的分片状态置为 LeaderDeleteStatus.
	b. 将新节点的分片状态改为 LeaderCreateStatus, 新节点角色是 Follower
	c. 新节点服务状态置为不可用
	d. 生成临时配置，入库、 check 服务状态、推上线
	e. check 主从同步状态
	f. LeaderCreateStatus 状态的服务改为可用(并且状态改变)，LeaderDeleteStatus 改为不可用 删除, 重新入库、check 服务状态、推上线
	h. 完成扩容

附加信息：【主机器扩容】记录删除的分片、和添加分片的信息

3. 主节点挂掉(一键灾备)

	a. 点击 将 选择对应灾备的分组进行主从分片切换
	b. 将 LeaderStatus 改为 LeaderDown
	c. 将原来的 Follower 的灾备分片改为 LeaderTemp
	d. 将 LeaderDown 全部删除掉
	d. 入库、推上线
	e. 进入灾备状态，灾备状态只允许加从节点

附加信息：【灾备】灾备的主分片，从分片

4. 主节点恢复

	a. 判断要切换的主分片的数据同步问题
	b. 切换
	d. 入库、推上线

附加信息：主节点

5. 从节点替换

	a. 节点改为不可用
	b. 加入不可用轮询机制，检查数据同步问题,
			静默不可用期： 不断发送不可用请求
			过了静默期： 去检查数据同步是否完成，如果完成改为可用

附加信息： 更换节点，静默期设置
*/

type ShardStore struct {
	Id          uint32             `gorm:"primary_key"`
	ServiceName string             `gorm:"type:varchar(256);not null;"`
	Version     string             `gorm:"type:varchar(256);not null;"`
	Data        string             `gorm:"type:text;not null;"`
	Status      ShardVersionStatus `gorm:"int(11); not null;"`
	CreatedAt   time.Time
}

type NodeVersionData struct {
	LeaderShardList   []uint32 `json:"LeaderShardList"`
	FollowerShardList []uint32 `json:"FollowerShardList"`
	NodeId            uint32   `json:"NodeId"`
	IsEdgeNode        bool     `json:"IsEdgeNode"`
}

type GroupVersionData struct {
	GroupName string            `json:"GroupName"`
	Dc        string            `json:"Dc"`
	Nodes     []NodeVersionData `json:"Nodes"`
}

type DataCenterData struct {
	Name        string `json:"Name"`
	ShardNumber uint32 `json:"ShardNumber"`
}

type ShardVersionData struct {
	ClusterName string             `json:"ClusterName"`
	ShardNumber uint32             `json:"ShardNumber"`
	Dcs         []DataCenterData   `json:"Dcs"`
	Groups      []GroupVersionData `json:"Groups"`
}

// 需要存储到 mysql
type Shard struct {
	GroupName string
	NodeId    uint32
	ShardId   uint32
	Role      ShardRole
	State     ShardState // 分片状态，需要落到数据库持久化
}

type ShardList struct {
	Relations map[uint32]ShardRelation `json:"relations"`
	Shards    map[string]ShardStatus   `json:"shards"`
	Groups    map[string]GroupShardDto `json:"groupShards"`
}
type GroupShardDto map[uint32][]string

type NodeInfo struct {
	GroupName  string
	NodeId     uint32
	Host       string
	Port       uint16
	Weight     uint32
	Master     bool
	Active     uint16
	Shards     []Shard
	IsEdgeNode bool
}

type ShardManager struct {
	ctx           *context.Context
	serviceName   string
	clusterName   string
	shardTotal    uint32
	groups        map[string]map[uint32]*NodeInfo // 本地暂存的临时节点信息
	dcs           []Dc
	groupToDc     map[string]string
	activeVersion string
	statusManager *ShardStatusManager // 从线上拉取的节点信息
	controlStatus ControlStatus
}

var (
	shardManager     *ShardManager
	shardManagerOnce sync.Once
)

func GetShardModel(ctx *context.Context, info *ClusterInfo) *ShardManager {
	shardManagerOnce.Do(func() {
		instance := &ShardManager{
			ctx:           ctx,
			serviceName:   ctx.GetConfig().ServiceName(),
			clusterName:   info.Name,
			shardTotal:    info.ShardTotal,
			dcs:           info.Dcs,
			groups:        make(map[string]map[uint32]*NodeInfo),
			groupToDc:     make(map[string]string),
			statusManager: NewShardStatusManager(ctx),
		}
		shardManager = instance
		UpdateClusterInfo(info)
		// shardManager.init()
	})
	return shardManager
}

// 必须保证对象已经初始化
func GetShardModelNoSafe() *ShardManager {
	return shardManager
}

func UpdateClusterInfo(info *ClusterInfo) {
	// todo 详细的实现节点变更的逻辑
	groups := make(map[string]map[uint32]*NodeInfo)
	nodes := make([]*NodeHostInfo, 0)
	groupToDc := make(map[string]string)
	for _, group := range info.Groups {
		groupToDc[group.Name] = group.DcName
		groupInfo := make(map[uint32]*NodeInfo)
		for _, node := range group.Nodes {
			nodeInfo := &NodeInfo{
				GroupName:  group.Name,
				NodeId:     node.NodeId,
				Weight:     node.Weight,
				Master:     node.Master,
				Active:     node.Active,
				Shards:     node.Shards,
				IsEdgeNode: node.IsEdgeNode,
			}
			groupInfo[nodeInfo.NodeId] = nodeInfo
			nodes = append(nodes, &NodeHostInfo{
				GroupName: group.Name,
				NodeId:    node.NodeId,
				Host:      node.Host,
				Port:      node.Port,
			})
		}
		groups[group.Name] = groupInfo
	}

	shardManager.clusterName = info.Name
	shardManager.shardTotal = info.ShardTotal
	shardManager.groups = groups
	shardManager.groupToDc = groupToDc
	shardManager.dcs = info.Dcs
	shardManager.statusManager.SetNodes(nodes)
}

func (shardManager *ShardManager) AssignShards() *common.Status {
	for _, nodes := range shardManager.groups {
		leaderNodes := make([]*NodeInfo, 0)
		followerNodes := make([]*NodeInfo, 0)
		for _, node := range nodes {
			if node.Master {
				leaderNodes = append(leaderNodes, node)
			} else {
				followerNodes = append(followerNodes, node)
			}
		}

		if len(leaderNodes) > 0 {
			shardManager.assignShardByRole(leaderNodes)
		}
		if len(followerNodes) > 0 {
			shardManager.assignShardByRole(followerNodes)
		}
	}
	return common.StatusOk()
}

func (shardManager *ShardManager) GetShards() *ShardList {
	shards, relations := shardManager.statusManager.GetShardStatus()
	result := &ShardList{
		Relations: relations,
		Shards:    shards,
		Groups:    shardManager.convertToDto(),
	}
	return result
}

func (shardManager *ShardManager) GetCurrentVersionData() (string, *common.Status) {
	shardStore, status := shardManager.convertNodeShardsToShardStore()
	currentVersionData := shardStore.Data
	return currentVersionData, status
}

func (shardManager *ShardManager) init() *common.Status {
	status := shardManager.loadActiveShards()
	if status.Code() == common.ShardVersionNotFound {
		shardManager.AssignShards()
		shardManager.storeShards()
	}
	return status
}

func (shardManager *ShardManager) convertVersionToNodeShards(version ShardStore) *common.Status {
	shardVersionData := ShardVersionData{}
	err := json.Unmarshal([]byte(version.Data), &shardVersionData)
	if err != nil {
		shardManager.ctx.Log().Error(err.Error())
		return common.StatusWithError(err)
	}

	if shardVersionData.ClusterName != shardManager.clusterName {
		return common.StatusWithMessage(common.ERROR, "目前分片信息不是当前集群的配置")
	}
	if shardVersionData.ShardNumber != shardManager.shardTotal {
		return common.StatusWithMessage(common.ERROR, "分片总数不一致，需要重新分配")
	}

	// 清除现有的 shard 信息
	for _, group := range shardManager.groups {
		for _, node := range group {
			node.Shards = make([]Shard, 0)
		}
	}

	for _, group := range shardVersionData.Groups {
		groupInfo, ok := shardManager.groups[group.GroupName]
		if !ok {
			continue
		}
		for _, node := range group.Nodes {
			nodeInfo, ok := groupInfo[node.NodeId]
			if !ok {
				continue
			}
			for _, leaderShardId := range node.LeaderShardList {
				shard := Shard{
					GroupName: group.GroupName,
					NodeId:    node.NodeId,
					ShardId:   leaderShardId,
					Role:      Leader,
				}
				nodeInfo.Shards = append(nodeInfo.Shards, shard)
			}
			for _, followerShardId := range node.FollowerShardList {
				shard := Shard{
					GroupName: group.GroupName,
					NodeId:    node.NodeId,
					ShardId:   followerShardId,
					Role:      Follower,
				}
				nodeInfo.Shards = append(nodeInfo.Shards, shard)
			}
		}
	}

	return common.StatusOk()
}

func (shardManager *ShardManager) getVersion() string {
	now := time.Now()
	return fmt.Sprintf("%d_%d_%d_%d_%d_%d", now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second())
}

func (shardManager *ShardManager) convertNodeShardsToVersionData() (ShardVersionData, *common.Status) {
	groups := make([]GroupVersionData, 0)
	groupNames := make([]string, 0)
	for k := range shardManager.groups {
		groupNames = append(groupNames, k)
	}
	sort.Strings(groupNames)
	for _, groupName := range groupNames {
		group := GroupVersionData{
			GroupName: groupName,
		}
		groupNodes := shardManager.groups[groupName]
		nodeIds := make([]int, 0)
		for nodeId := range groupNodes {
			nodeIds = append(nodeIds, int(nodeId))
		}
		sort.Ints(nodeIds)
		nodes := make([]NodeVersionData, 0)
		for _, nodeId := range nodeIds {
			node := groupNodes[uint32(nodeId)]
			if node.GroupName != groupName || node.Active == uint16(NodeInactive) {
				continue
			}
			leaderList := make([]uint32, 0)
			followerList := make([]uint32, 0)
			for _, shard := range node.Shards {
				if shard.Role == Leader {
					leaderList = append(leaderList, shard.ShardId)
				} else {
					followerList = append(followerList, shard.ShardId)
				}
			}
			nodeData := NodeVersionData{
				LeaderShardList:   leaderList,
				FollowerShardList: followerList,
				NodeId:            node.NodeId,
				IsEdgeNode:        node.IsEdgeNode,
			}
			nodes = append(nodes, nodeData)
		}
		group.Nodes = nodes

		dc, ok := shardManager.groupToDc[groupName]
		if !ok {
			dc = "default"
		}
		group.Dc = dc
		groups = append(groups, group)
	}

	dcs := make([]DataCenterData, len(shardManager.dcs))
	for idx, dc := range shardManager.dcs {
		dcs[idx] = DataCenterData{
			Name:        dc.Name,
			ShardNumber: uint32(dc.ShardNumber),
		}
	}

	shardVersionData := ShardVersionData{
		ClusterName: shardManager.clusterName,
		ShardNumber: shardManager.shardTotal,
		Groups:      groups,
		Dcs:         dcs,
	}

	return shardVersionData, common.StatusOk()
}

func (shardManager *ShardManager) convertNodeShardsToShardStore() (ShardStore, *common.Status) {
	version := shardManager.getVersion()
	versionData := ShardStore{
		ServiceName: shardManager.serviceName,
		Version:     version,
		Status:      Active,
		CreatedAt:   time.Now(),
	}

	shardVersionData, status := shardManager.convertNodeShardsToVersionData()
	if status != nil && !status.Ok() {
		return versionData, status
	}

	data, err := json.Marshal(shardVersionData)
	if err != nil {
		return versionData, common.StatusWithError(err)
	}
	versionData.Data = string(data)
	return versionData, common.StatusOk()
}

func (shardManager *ShardManager) loadActiveShards() *common.Status {
	version := ShardStore{}
	db := shardManager.ctx.Db().Model(&ShardStore{})
	err := db.Where(map[string]interface{}{"service_name": shardManager.serviceName, "status": Active}).First(&version).Error
	if gorm.IsRecordNotFoundError(err) {
		return common.StatusError(common.ShardVersionNotFound)
	}
	if err != nil {
		return common.StatusWithError(err)
	}
	return shardManager.convertVersionToNodeShards(version)
}

func (shardManager *ShardManager) storeShards() *common.Status {
	shardStore, err := shardManager.convertNodeShardsToShardStore()
	if err != nil && !err.Ok() {
		return err
	}

	db := shardManager.ctx.Db().Model(&ShardStore{})
	tx := db.Begin()
	if shardManager.activeVersion != "" {
		err := tx.Where(map[string]interface{}{"service_name": shardManager.serviceName, "version": shardManager.activeVersion}).
			Update("status", Default).Error
		if err != nil {
			tx.Rollback()
			return common.StatusWithError(err)
		}
	}
	createErr := tx.Create(&shardStore).Error
	if createErr != nil {
		tx.Rollback()
		return common.StatusWithError(createErr)
	}
	tx.Commit()
	shardManager.activeVersion = shardStore.Version
	return common.StatusOk()
}

func (shardManager *ShardManager) exchangeShards(version string, shardStore *ShardStore) *common.Status {
	db := shardManager.ctx.Db().Model(&ShardStore{})
	tx := db.Begin()

	err := tx.Where(map[string]interface{}{"service_name": shardManager.serviceName, "version": shardManager.activeVersion}).
		Update("status", Default).Error
	if err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}

	err = tx.Where(map[string]interface{}{"service_name": shardManager.serviceName, "version": version}).
		Update("status", Active).Error
	if err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	shardManager.activeVersion = shardStore.Version
	return shardManager.loadActiveShards()
}

func (shardManager *ShardManager) shardNumber(nodes []*NodeInfo) map[int32]uint32 {
	numbers := make(map[int32]uint32)
	var totalWeight float64 = 0
	for _, nodeInfo := range nodes {
		totalWeight += float64(nodeInfo.Weight)
	}

	var remainNumber = int32(shardManager.shardTotal)
	var lastNodeId int32 = -1
	for _, nodeInfo := range nodes {
		rate := float64(nodeInfo.Weight) / totalWeight
		number := uint32(math.Ceil(float64(shardManager.shardTotal) * rate))
		lastNodeId = int32(nodeInfo.NodeId)
		remainNumber -= int32(number)
		numbers[int32(nodeInfo.NodeId)] = number
	}

	if remainNumber > 0 && lastNodeId >= 0 {
		numbers[lastNodeId] += uint32(remainNumber)
	}
	return numbers
}

func (shardManager *ShardManager) assignShardByRole(nodes []*NodeInfo) {
	// 获取当前缺失的分片
	shardNumbers := shardManager.shardNumber(nodes)

	remainShards := make([]uint32, 0)
	assignShards := make(map[uint32]uint32)
	for _, nodeInfo := range nodes {
		moveNumber := len(nodeInfo.Shards) - int(shardNumbers[int32(nodeInfo.NodeId)])
		if moveNumber <= 0 {
			continue
		}
		for _, shard := range nodeInfo.Shards[:len(nodeInfo.Shards)-moveNumber] { // ?????
			remainShards = append(remainShards, shard.ShardId)
		}

		nodeInfo.Shards = nodeInfo.Shards[0 : len(nodeInfo.Shards)-moveNumber]
		for _, shard := range nodeInfo.Shards {
			assignShards[shard.ShardId]++
		}
	}

	var shardId uint32 = 0
	for ; shardId < shardManager.shardTotal; shardId++ {
		if _, ok := assignShards[shardId]; ok {
			continue
		}
		remainShards = append(remainShards, shardId)
	}

	for _, nodeInfo := range nodes {
		moveNumber := len(nodeInfo.Shards) - int(shardNumbers[int32(nodeInfo.NodeId)])
		if moveNumber >= 0 || len(remainShards) == 0 {
			continue
		}

		if -moveNumber > len(remainShards) {
			moveNumber = -len(remainShards)
		}
		for _, shardId := range remainShards[0:-moveNumber] {
			var role ShardRole
			if nodeInfo.Master {
				role = Leader
			} else {
				role = Follower
			}
			nodeInfo.Shards = append(nodeInfo.Shards, Shard{
				GroupName: nodeInfo.GroupName,
				NodeId:    nodeInfo.NodeId,
				ShardId:   shardId,
				Role:      role,
			})
		}
		remainShards = remainShards[-moveNumber:]
	}
}

func (shardManager *ShardManager) convertToDto() map[string]GroupShardDto {
	groups := make(map[string]GroupShardDto)
	for groupName, groupNodes := range shardManager.groups {
		group := GroupShardDto{}
		for _, node := range groupNodes {
			nodeShards := make([]string, 0)
			if node.GroupName != groupName {
				continue
			}
			for _, shard := range node.Shards {
				nodeShards = append(nodeShards, GetShardHash(GetNodeHash(groupName, node.NodeId), shard.ShardId))
			}
			group[node.NodeId] = nodeShards
		}
		groups[groupName] = group
	}
	return groups
}
