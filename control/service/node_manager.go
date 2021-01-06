package service

import (
	"encoding/json"
	"fmt"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"strconv"
	"strings"
	"sync"

	"github.com/jinzhu/gorm"
)

type NodeStatus uint16

const (
	NodeStatusDisabled NodeStatus = 0
	NodeStatusEnabled  NodeStatus = 1
)

type NodeActiveState uint16

const (
	NodeInactive NodeActiveState = 0
	NodeActive   NodeActiveState = 1
)

type Node struct {
	Id                uint32           `gorm:"primary_key"`
	NodeId            uint32           `gorm:"type:int(32);not null"`
	Active            *NodeActiveState `gorm:"type:int(16);not null;default 0"` // 0-Inactive, 1-Active
	Status            *NodeStatus      `gorm:"type:int(16);not null;default 1"` // 0-disabled, 1-enabled
	Host              string           `gorm:"type:varchar(256);not null"`
	Port              uint16           `gorm:"type:int(32);not null"`
	Weight            *uint32          `gorm:"type:int(32);not null"`
	Master            *bool            `gorm:"type:boolean;not null;default:false"` // false-NotMaster; true-Master
	IsEdgeNode        *bool            `gorm:"type:boolean;not null;default:false"`
	Desc              string           `gorm:"type:varchar(256)"`
	GroupId           uint32           `gorm:"type:int(32);not null;"`
	GroupInfo         Group            `gorm:"ForeignKey:GroupId"`
	ConfigId          uint32           `gorm:"type:int(32);not null"`
	Config            NodeConfig       `gorm:"ForeignKey:ConfigId"`
	LeaderShardList   *string          `gorm:"type:text"`
	FollowerShardList *string          `gorm:"type:text"`
	BindTables        []Table          `gorm:"many2many:table_edge_nodes"`
	AnsibleConfigId   uint32           `gorm:"type:int(32);not null;"`
	AnsibleConfig     AnsibleConfig    `gorm:"ForeignKey:AnsibleConfigId"`
}

type NodeManager struct {
	ctx     *context.Context
	cluster *ClusterInfo
}

var (
	nodeManager     *NodeManager
	nodeManagerOnce sync.Once
)

func GetNodeModel(ctx *context.Context) *NodeManager {
	nodeManagerOnce.Do(func() {
		instance := &NodeManager{
			ctx: ctx,
		}
		groupModel := NewGroupModel(ctx)
		groupModel.SubscribeGroupChange(instance.UpdateClusterInfo)
		_, instance.cluster = instance.GetClusterInfo()
		nodeManager = instance
	})
	return nodeManager
}

type GroupInfo struct {
	Id     uint32
	Name   string
	DcName string
	Alias  string
	Nodes  []NodeInfo
}

type ClusterInfo struct {
	Name       string
	ShardTotal uint32
	Dcs        []Dc
	Groups     []GroupInfo
}

type ShardsListStruct struct {
	Table  string `json:"Table"`
	Shards string `json:"Shards"`
}

type ShardsStruct struct {
	ShardsList []ShardsListStruct `json:"ShardsList"`
}

func (node *NodeManager) UpdateClusterInfo() *common.Status {
	status, cluster := node.GetClusterInfo()
	node.cluster = cluster
	UpdateClusterInfo(node.cluster)
	return status
}

func (nodeManager *NodeManager) GetClusterInfo() (*common.Status, *ClusterInfo) {
	clusterInfo := ClusterInfo{}

	clusterModel := NewClusterModel(nodeManager.ctx)
	clusterListParams := params.ClusterListParams{
		Page:  0,
		Limit: 0,
	}
	clusters, _ := clusterModel.List(clusterListParams, false)
	if len(clusters) == 0 {
		return common.StatusError(common.ClusterNotExist), &clusterInfo
	}
	clusterInfo.ShardTotal = clusters[0].ShardTotal
	clusterInfo.Name = clusters[0].Name

	dcModel := NewDcModel(nodeManager.ctx)
	dcListParams := params.DcListParams{
		Page:  0,
		Limit: 0,
	}

	dcs, _ := dcModel.List(dcListParams, false)
	clusterInfo.Dcs = dcs

	groupModel := NewGroupModel(nodeManager.ctx)
	groupListParams := params.GroupListParams{
		Page:  0,
		Limit: 0,
	}
	groups, _ := groupModel.List(groupListParams, false)
	groupInfos := make([]GroupInfo, 0)
	for _, group := range groups {
		nodeInfos := make([]NodeInfo, 0)
		nodeListParams := params.NodeListParams{
			Page:    0,
			Limit:   0,
			GroupId: group.Id,
		}
		nodes, _ := nodeManager.list(nodeListParams, false)
		for _, node := range nodes {
			nodeInfos = append(nodeInfos, NodeInfo{
				GroupName:  group.Name,
				NodeId:     node.NodeId,
				Host:       node.Host,
				Port:       node.Port,
				Weight:     *node.Weight,
				Master:     *node.Master,
				IsEdgeNode: *node.IsEdgeNode,
				Active:     uint16(*node.Active),
				Shards:     nodeManager.shardStrListToSlice(node),
			})
		}

		groupInfos = append(groupInfos, GroupInfo{
			Id:     group.Id,
			Name:   group.Name,
			DcName: group.Dc.Name,
			Alias:  group.Alias,
			Nodes:  nodeInfos,
		})
		clusterInfo.Groups = groupInfos
	}

	return common.StatusOk(), &clusterInfo
}

func (node *NodeManager) Store(params params.NodeParams) *common.Status {
	status := node.store(params)
	if !status.Ok() {
		return status
	}
	return node.UpdateClusterInfo()
}

func (node *NodeManager) BatchStore(params params.NodeBatchStoreParams) *common.Status {
	status := node.batchStore(params)
	if !status.Ok() {
		return status
	}
	return node.UpdateClusterInfo()
}

func (nodeManager *NodeManager) ListNode() (*common.Status, []params.NodeGroup) {
	nodeGroups := make([]params.NodeGroup, 0)
	attrs := make([]params.NodeInfoAttr, 0)
	attrs = append(attrs, params.NodeInfoAttr{
		Name:  "内存",
		Value: "80GB",
	})
	attrs = append(attrs, params.NodeInfoAttr{
		Name:  "线程数",
		Value: "200",
	})
	attrs = append(attrs, params.NodeInfoAttr{
		Name:  "服务分片",
		Value: "200/400",
	})

	groupModle := NewGroupModel(nodeManager.ctx)
	groupListParams := params.GroupListParams{}
	groups, _ := groupModle.List(groupListParams, false)
	shardMetricManagerMod := GetShardMetricsManager(nodeManager.ctx)
	for _, group := range groups {
		nodeInfos := make([]params.NodeInfo, 0)
		nodeListParams := params.NodeListParams{
			Page:    0,
			Limit:   0,
			GroupId: group.Id,
		}
		nodes, _ := nodeManager.list(nodeListParams, false)
		for _, node := range nodes {
			isAvailable := shardMetricManagerMod.IsNodeAvailable(group.Name, node.NodeId)
			readKps := uint64(shardMetricManagerMod.GetNodeMetricValue(group.Name, node.NodeId, ReadKpsMin1))
			writeKps := uint64(shardMetricManagerMod.GetNodeMetricValue(group.Name, node.NodeId, WriteKpsMin1))
			nodeInfos = append(nodeInfos, params.NodeInfo{
				Id:                   node.Id,
				NodeId:               node.NodeId,
				Active:               (*uint16)(node.Active),
				Status:               (*uint16)(node.Status),
				Host:                 node.Host,
				Port:                 node.Port,
				Weight:               node.Weight,
				Master:               *node.Master,
				IsEdgeNode:           *node.IsEdgeNode,
				ShardNumber:          0,
				AvailableShardNumber: 0,
				Attrs:                attrs,
				Desc:                 node.Desc,
				GroupId:              node.GroupId,
				GroupName:            node.GroupInfo.Name,
				Dc:                   group.Dc.Name,
				ConfigId:             node.ConfigId,
				ConfigName:           node.Config.Name,
				AnsibleConfigId:      node.AnsibleConfigId,
				AnsibleConfigName:    node.AnsibleConfig.Name,
				LeaderShardList:      node.LeaderShardList,
				FollowerShardList:    node.FollowerShardList,
				IsAvailable:          isAvailable,
				ReadKps:              readKps,
				WriteKps:             writeKps,
			})
		}
		nodeGroups = append(nodeGroups, params.NodeGroup{
			Id:        group.Id,
			GroupName: group.Name,
			Dc:        group.Dc.Name,
			DescName:  group.Alias,
			Nodes:     nodeInfos,
		})
	}
	return common.StatusOk(), nodeGroups
}

func (node *NodeManager) UpdateNode(params params.NodeParams) *common.Status {
	status := node.update(params)
	if !status.Ok() {
		return status
	}
	return node.UpdateClusterInfo()
}

func (node *NodeManager) ChangeRole(params params.NodeChangeRoleParams) *common.Status {
	status := node.changeRole(params)
	if !status.Ok() {
		return status
	}
	return node.UpdateClusterInfo()
}

func (node *NodeManager) DeleteNode(params params.NodeDeleteParams) *common.Status {
	status := node.delete(params)
	if !status.Ok() {
		return status
	}
	return node.UpdateClusterInfo()
}

func (node *NodeManager) BatchDeleteNode(params params.NodeBatchDeleteParams) *common.Status {
	status := node.batchDelete(params)
	if !status.Ok() {
		return status
	}
	return node.UpdateClusterInfo()
}

func (node *NodeManager) store(params params.NodeParams) *common.Status {
	db := node.ctx.Db()
	tx := db.Begin()
	if status := node.checkDupNode(tx, params.GroupId, params.Id, params.Host, params.Port, params.NodeId); !status.Ok() {
		tx.Rollback()
		return status
	}

	nodeAdd := &Node{
		NodeId:            params.NodeId,
		Active:            (*NodeActiveState)(params.Active),
		Status:            (*NodeStatus)(params.Status),
		Host:              params.Host,
		Port:              params.Port,
		Weight:            params.Weight,
		Master:            params.Master,
		IsEdgeNode:        params.IsEdgeNode,
		Desc:              params.Desc,
		GroupId:           params.GroupId,
		ConfigId:          params.ConfigId,
		AnsibleConfigId:   params.AnsibleConfigId,
		LeaderShardList:   params.LeaderShardList,
		FollowerShardList: params.FollowerShardList,
	}

	if err := tx.Create(nodeAdd).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (node *NodeManager) batchStore(param params.NodeBatchStoreParams) *common.Status {
	db := node.ctx.Db()
	tx := db.Begin()

	sql := "INSERT INTO `nodes` (`node_id`, `active`, `status`, `host`, `port`, `weight`, `master`, `is_edge_node`, `desc`, `group_id`, `config_id`, `ansible_config_id`, `leader_shard_list`, `follower_shard_list`) VALUES "
	for index, nodeInfo := range param.Nodes {
		if status := node.checkDupNode(tx, nodeInfo.GroupId, nodeInfo.Id, nodeInfo.Host,
			nodeInfo.Port, nodeInfo.NodeId); !status.Ok() {
			tx.Rollback()
			return status
		}

		sql += fmt.Sprintf("(%d, %d, %d, '%s', %d, %d, %t, %t, '%s', %d, %d, %d, '%s', '%s')",
			nodeInfo.NodeId, *nodeInfo.Active, *nodeInfo.Status, nodeInfo.Host, nodeInfo.Port, *nodeInfo.Weight, *nodeInfo.Master,
			*nodeInfo.IsEdgeNode, nodeInfo.Desc, nodeInfo.GroupId, nodeInfo.ConfigId, nodeInfo.AnsibleConfigId, *nodeInfo.LeaderShardList, *nodeInfo.FollowerShardList)
		if len(param.Nodes)-1 == index {
			sql += ";"
		} else {
			sql += ","
		}
	}

	if err := tx.Exec(sql).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (node *NodeManager) list(params params.NodeListParams, isTotal bool) ([]Node, uint32) {
	db := node.ctx.Db().Model(&Node{}).Where(&Node{GroupId: params.GroupId, IsEdgeNode: params.IsEdgeNode})
	var total uint32
	var nodes []Node
	if isTotal {
		db.Count(&total)
	}
	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		db = db.Offset(offset).Limit(params.Limit)
	}
	db.Preload("GroupInfo").Preload("Config").Preload("AnsibleConfig").Find(&nodes)
	return nodes, total
}

func (node *NodeManager) update(params params.NodeParams) *common.Status {
	db := node.ctx.Db()
	tx := db.Begin()
	if status := node.checkDupNode(tx, params.GroupId, params.Id, params.Host, params.Port, params.NodeId); status.Code() != common.NodeDup {
		tx.Rollback()
		return common.StatusError(common.NodeNotExist)
	}
	if !*params.IsEdgeNode {
		if err := tx.Model(&Node{Id: params.Id}).Association("BindTables").Clear().Error; err != nil {
			tx.Rollback()
			return common.StatusWithError(err)
		}
	}
	if err := tx.Model(&Node{Id: params.Id}).UpdateColumns(Node{
		NodeId:            params.NodeId,
		Active:            (*NodeActiveState)(params.Active),
		Status:            (*NodeStatus)(params.Status),
		Host:              params.Host,
		Port:              params.Port,
		Weight:            params.Weight,
		Master:            params.Master,
		IsEdgeNode:        params.IsEdgeNode,
		Desc:              params.Desc,
		GroupId:           params.GroupId,
		ConfigId:          params.ConfigId,
		AnsibleConfigId:   params.AnsibleConfigId,
		LeaderShardList:   params.LeaderShardList,
		FollowerShardList: params.FollowerShardList,
	}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (node *NodeManager) changeRole(params params.NodeChangeRoleParams) *common.Status {
	db := node.ctx.Db()
	tx := db.Begin()

	var nodes []Node
	if err := tx.Model(&Node{}).Where(&Node{Id: params.Id, GroupId: params.GroupId}).Find(&nodes).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}

	for _, nodeInfo := range nodes {
		if *nodeInfo.Master == *params.Master {
			continue
		}

		if *nodeInfo.IsEdgeNode && *params.Master {
			tx.Rollback()
			return common.StatusError(common.NodeSetEdgeNodeAsMasterFail)
		}

		if err := tx.Model(&Node{Id: nodeInfo.Id}).UpdateColumns(&Node{
			Master:            params.Master,
			LeaderShardList:   nodeInfo.FollowerShardList,
			FollowerShardList: nodeInfo.LeaderShardList,
		}).Error; err != nil {
			tx.Rollback()
			return common.StatusWithError(err)
		}
	}

	tx.Commit()
	return common.StatusOk()
}

func (node *NodeManager) delete(params params.NodeDeleteParams) *common.Status {
	db := node.ctx.Db()
	tx := db.Begin()
	if err := tx.Model(&Node{Id: params.Id}).Association("BindTables").Clear().Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Unscoped().Delete(&Node{Id: params.Id}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (node *NodeManager) batchDelete(params params.NodeBatchDeleteParams) *common.Status {
	db := node.ctx.Db()
	tx := db.Begin()
	for _, id := range params.Ids {
		if err := tx.Model(&Node{Id: id}).Association("BindTables").Clear().Error; err != nil {
			tx.Rollback()
			return common.StatusWithError(err)
		}
		if err := tx.Unscoped().Delete(&Node{Id: id}).Error; err != nil {
			tx.Rollback()
			return common.StatusWithError(err)
		}
	}
	tx.Commit()
	return common.StatusOk()
}

func (node *NodeManager) checkDupNode(tx *gorm.DB, groupId uint32, id uint32,
	host string, port uint16, nodeId uint32) *common.Status {
	var count int
	// check id
	if id != 0 {
		if err := tx.Model(&Node{}).Where(&Node{Id: id}).Count(&count).Error; err != nil {
			return common.StatusWithError(err)
		}
		if count != 0 {
			return common.StatusError(common.NodeDup)
		}
	}

	// check nodeId
	count = 0
	if err := tx.Model(&Node{}).Where(&Node{NodeId: nodeId, GroupId: groupId}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if count != 0 {
		return common.StatusError(common.NodeIdDup)
	}

	// check address
	count = 0
	if err := tx.Model(&Node{}).Where(&Node{Host: host, Port: port}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if count != 0 {
		return common.StatusError(common.NodeAddressDup)
	}

	return common.StatusOk()
}

func (node *NodeManager) GetShardTotal() (uint32, *common.Status) {
	if node.cluster == nil {
		return 0, common.StatusError(common.ClusterParserError)
	}
	return node.cluster.ShardTotal, common.StatusOk()
}

func extractUniqueShards(shardsStructString string) []string {
	// 尝试解析json格式的分片列表，如果失败，则按逗号分隔的格式解析
	inShardStructJsonBytes := []byte(shardsStructString)
	var shardsStruct ShardsStruct
	if err := json.Unmarshal(inShardStructJsonBytes, &shardsStruct); err != nil {
		if shardsStructString == "" {
			return make([]string, 0)
		}
		return strings.Split(shardsStructString, ",")
	}

	// 去重json格式（按表分类）的分片列表
	var uniqueShards []string
	shardsMap := make(map[string]bool)
	for _, shardList := range shardsStruct.ShardsList {
		if shardList.Shards == "" {
			continue
		}
		shards := strings.Split(shardList.Shards, ",")
		for _, shardId := range shards {
			if _, ok := shardsMap[shardId]; !ok {
				uniqueShards = append(uniqueShards, shardId)
				shardsMap[shardId] = true
			}
		}
	}
	return uniqueShards
}

func (node *NodeManager) shardStrListToSlice(nodeInfo Node) []Shard {
	shards := make([]Shard, 0)
	leaderShards := extractUniqueShards(*nodeInfo.LeaderShardList)
	for _, shardId := range leaderShards {
		numShardId, err := strconv.ParseUint(strings.TrimSpace(shardId), 10, 32)
		if err != nil {
			continue
		}
		shards = append(shards, Shard{
			GroupName: nodeInfo.GroupInfo.Name,
			NodeId:    nodeInfo.NodeId,
			ShardId:   uint32(numShardId),
			Role:      Leader,
			State:     LeaderStatus,
		})
	}

	followerShards := extractUniqueShards(*nodeInfo.FollowerShardList)

	for _, shardId := range followerShards {
		numShardId, err := strconv.ParseUint(strings.TrimSpace(shardId), 10, 32)
		if err != nil {
			continue
		}
		shards = append(shards, Shard{
			GroupName: nodeInfo.GroupInfo.Name,
			NodeId:    nodeInfo.NodeId,
			ShardId:   uint32(numShardId),
			Role:      Follower,
			State:     FollowerStatus,
		})
	}
	return shards
}

func (node *NodeManager) shardSliceToStrList(shards []Shard) (string, string) {
	leaderShards := make([]string, 0)
	followerShards := make([]string, 0)

	for _, shard := range shards {
		if shard.Role == Leader {
			leaderShards = append(leaderShards, strconv.FormatUint(uint64(shard.ShardId), 10))
		} else if shard.Role == Follower {
			followerShards = append(followerShards, strconv.FormatUint(uint64(shard.ShardId), 10))
		}
	}

	leaderShardStrList := strings.Join(leaderShards, ",")
	followerShardStrList := strings.Join(followerShards, ",")
	return leaderShardStrList, followerShardStrList
}
