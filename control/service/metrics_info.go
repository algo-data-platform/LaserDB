package service

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	laserContext "laser-control/context"
	"laser-control/params"
	"math"
	"net/http"
	"sort"
	"strconv"
	"sync"
	"time"

	"github.com/algo-data-platform/LaserDB/sdk/go/laser_client/lib"
	"github.com/zentures/cityhash"
	// "real.path.com/ads/ads_common_go/common/cityhash"
	// "real.path.com/ads/laser_client/lib"
)

const SYNC_RESULT_TIMEOUT_SECONDS = 30

/* 解析shard/list所需结构 */
type ShardPartitionInfo struct {
	DbInfo struct {
		ReplicateLag int64  `json:"ReplicateLag"`
		SeqNo        uint64 `json:"SeqNo"`
	} `json:"DbInfo"`
	DeltaVersions []string `json:"DeltaVersions"`
	PartitionId   uint32   `json:"PartitionId"`
	Hash          int64    `json:"Hash"`
	DatabaseName  string   `json:"DatabaseName"`
	TableName     string   `json:"TableName"`
	Role          string   `json:"Role"`
	BaseVersion   string   `json:"BaseVersion"`
	Size          uint64   `json:"Size"`
	ReadKps       uint64   `json:"ReadKps"`
	WriteKps      uint64   `json:"WriteKps"`
	ReadBytes     uint64   `json:"ReadBytes"`
	WriteBytes    uint64   `json:"WriteBytes"`
}
type ShardPartitionInfos []ShardPartitionInfo

func (partitions ShardPartitionInfos) Len() int {
	return len(partitions)
}

func (partitions ShardPartitionInfos) Swap(i, j int) {
	partitions[i], partitions[j] = partitions[j], partitions[i]
}

func (partitions ShardPartitionInfos) Less(i, j int) bool {
	return partitions[i].Hash > partitions[j].Hash
}

type ShardListJson struct {
	Code    uint32 `json:"Code"`
	Message string `json:"Message"`
	Data    []struct {
		Status     string              `json:"Status"`
		Role       string              `json:"Role"`
		ShardId    uint32              `json:"ShardId"`
		Partitions ShardPartitionInfos `json:"Partitions"`
	} `json:"Data"`
}

/* 解析shard/list所需结构 */

/* 解析server/status所需结构 */
type MetricsValueInfo struct {
	defaultValue struct {
		Min15       float64 `json:"min_15"`
		Min5        float64 `json:"min_5"`
		Count       int64   `json:"count"`
		Min1        float64 `json:"min_1"`
		Avg         float64 `json:"avg"`
		MeanRate    float64 `json:"mean_rate"`
		Percent05   float64 `json:"percent_0.5"`
		Percent0999 float64 `json:"percent_0.999"`
		Percent095  float64 `json:"percent_0.95"`
		Tags        struct {
		} `json:"tags"`
		Percent075 float64 `json:"percent_0.75"`
		Percent099 float64 `json:"percent_0.99"`
		Percent098 float64 `json:"percent_0.98"`
	} `json:"default"`
}
type DefaultMetricsInfo map[string]MetricsValueInfo

type SystemTags struct {
	Value uint64 `json:"value"`
	Tags  struct {
		target_service_name string `json:"target_service_name"`
	} `json:"tags"`
}
type SystemValueInfo map[string]SystemTags

type RocksDbTableTags struct {
	Value float64 `json:"value"`
	Tags  struct {
		database_name string `json:"database_name"`
		role          string `json:"role"`
		table_name    string `table_name`
	} `json:"tags"`
}
type RocksDbTableInfo map[string](map[string]RocksDbTableTags)

type RocksDbTags struct {
	Value float64 `json:"value"`
	Tags  struct {
		rocksdb_stat_type string `json:"rocksdb_stat_type"`
	} `json:"tags"`
}
type RocksDbInfo map[string](map[string]RocksDbTags)

type ShardStatValue struct {
	Value float64 `json:"value"`
	Tags  struct {
		node_id  uint32 `json:"database_name"`
		property string `json:"role"`
		shard_id uint32 `table_name`
	} `json:"tags"`
}
type ShardStatInfo map[string]ShardStatValue

type ServerStatus struct {
	LaserService         DefaultMetricsInfo `json:"laser_service"`
	ServiceFrameworkHttp DefaultMetricsInfo `json:"service_framework.http"`
	ReplicationDb        DefaultMetricsInfo `json:"replication_db"`
	System               ShardStatInfo      `json:"system"`
	ServiceBase          struct {
		ServiceName  string `json:"service_name"`
		Host         string `json:"host"`
		Port         uint32 `json:"port"`
		ServerStatus string `json:"service_status"`
		PackageTag   string `json:"package_tag"`
	} `json:"service_base"`
	RocksdbTable RocksDbTableInfo `json:"rocksdb_table"`
	Rocksdb      RocksDbInfo      `json:"rocksdb"`
	Shard        struct {
		Stat ShardStatInfo `json:"stat"`
	} `json:"shard"`
	ServiceRouter DefaultMetricsInfo `json:"service_router"`
	HdfsMonitor   DefaultMetricsInfo `json:"hdfs_monitor"`
	RocksdbEngine DefaultMetricsInfo `json:"rocksdb_engine"`
	WdtReplicator DefaultMetricsInfo `json:"wdt_replicator"`
}

/* 解析server/status所需结构 */

/* 缓存中结构 */
type GroupShardMetrics struct {
	nodeInfo      map[uint64]NodeBasicInfo          // nodeIdHash : nodeInfo
	nodeShardInfo map[uint64](map[string]ShardInfo) // nodeIdHash : nodeMetrics(shardIdHash : shardMetricsValue)
}

type ShardInfo struct {
	ShardId         uint32
	ShardHash       string
	Role            ShardRole
	ServiceState    ShardServiceState
	SeqNo           uint64
	BaseVersionHash uint64
	Metrics         map[string]float64
	Partitions      []ShardPartitionInfo
}

type NodeBasicInfo struct {
	GroupName   string
	NodeId      uint32
	Host        string
	Port        uint16
	Master      *bool
	IsAvailable bool
}

type ShardPullNodeMetricsResult struct {
	groupName    string
	nodeId       uint32
	basicInfo    NodeBasicInfo
	shardMetrics map[string]ShardInfo
}

/* 缓存中结构 */

type MetricNameString string

const (
	WriteKpsMin1     MetricNameString = "write_kps_min_1"
	ReadKpsMin1      MetricNameString = "read_kps_min_1"
	WriteBytesMin1   MetricNameString = "write_bytes_min_1"
	ReadBytesMin1    MetricNameString = "read_bytes_min_1"
	LiveSstFilesSize MetricNameString = "live-sst-files-size"
)

var metricsType = []MetricNameString{
	WriteKpsMin1,
	ReadKpsMin1,
	WriteBytesMin1,
	ReadBytesMin1,
	LiveSstFilesSize,
}

const (
	Available ShardServiceState = iota + 1
	Unavailable
)

const (
	AvailableString   string = "available"
	UnavailableString string = "unavailable"
)

type ShardMetricsManager struct {
	sync.Mutex
	ctx          *laserContext.Context
	shardMetrics map[string]GroupShardMetrics // groupName : GroupShardMetrics
	cancel       context.CancelFunc
	ttlMs        uint32
}

var (
	shardMetricsManager     *ShardMetricsManager
	shardMetricsManagerOnce sync.Once
)

func GetShardMetricsManager(ctx *laserContext.Context) *ShardMetricsManager {
	shardMetricsManagerOnce.Do(func() {
		instance := &ShardMetricsManager{
			Mutex:        sync.Mutex{},
			ctx:          ctx,
			shardMetrics: make(map[string]GroupShardMetrics),
			ttlMs:        5000,
		}
		instance.start()
		shardMetricsManager = instance
	})
	return shardMetricsManager
}

func (shardMetricsManager *ShardMetricsManager) start() {
	ctx, cancel := context.WithCancel(context.Background())
	shardMetricsManager.cancel = cancel
	ttl := time.Duration(shardMetricsManager.ttlMs) * time.Millisecond
	ticker := time.NewTicker(ttl)
	go func() {
		for {
			select {
			case <-ticker.C:
				shardMetricsManager.syncShardMetrics()
			case <-ctx.Done():
				ticker.Stop()
				return
			}
		}
	}()
}

func (shardMetricsManager *ShardMetricsManager) syncShardMetrics() {
	var paramsGroup params.GroupListParams
	paramsGroup.ShowDetails = true
	group := NewGroupModel(shardMetricsManager.ctx)
	groupList, _ := group.List(paramsGroup, false)
	nodeNumber := 0
	for _, group := range groupList {
		nodeNumber += len(group.Nodes)
	}
	result := make(chan ShardPullNodeMetricsResult, nodeNumber)
	for _, group := range groupList {
		for _, node := range group.Nodes {
			go func(result chan ShardPullNodeMetricsResult, node Node, groupName string) {
				pulledNodeMetricsInfo := ShardPullNodeMetricsResult{
					groupName: groupName,
					nodeId:    node.NodeId,
				}
				nodeBasicInfo := NodeBasicInfo{
					GroupName: groupName,
					NodeId:    node.NodeId,
					Host:      node.Host,
					Port:      node.Port,
					Master:    node.Master,
				}

				nodeHash := GetNodeHash(groupName, node.NodeId)
				nodeShardMetrics := shardMetricsManager.getNodeShardInfoFromShardList(groupName, node)
				isNodeAvailable, nodeMetricsInfo := shardMetricsManager.getShardMetricsFromServerStatus(groupName,
					node, nodeHash, nodeShardMetrics.nodeShardInfo[nodeHash])

				nodeBasicInfo.IsAvailable = isNodeAvailable
				pulledNodeMetricsInfo.basicInfo = nodeBasicInfo
				pulledNodeMetricsInfo.shardMetrics = nodeMetricsInfo
				result <- pulledNodeMetricsInfo
			}(result, node, group.Name)
		}
	}

	for i := 0; i < nodeNumber; i++ {
		select {
		case shardMetricsInfo := <-result:
			nodeHash := GetNodeHash(shardMetricsInfo.groupName, shardMetricsInfo.nodeId)
			shardMetricsManager.Lock()
			groupMetrics, ok := shardMetricsManager.shardMetrics[shardMetricsInfo.groupName]
			if !ok {
				groupMetrics = GroupShardMetrics{
					nodeInfo:      make(map[uint64]NodeBasicInfo),
					nodeShardInfo: make(map[uint64]map[string]ShardInfo),
				}
				shardMetricsManager.shardMetrics[shardMetricsInfo.groupName] = groupMetrics
			}
			groupMetrics.nodeInfo[nodeHash] = shardMetricsInfo.basicInfo
			groupMetrics.nodeShardInfo[nodeHash] = shardMetricsInfo.shardMetrics
			shardMetricsManager.Unlock()
		case <-time.After(time.Second * time.Duration(SYNC_RESULT_TIMEOUT_SECONDS)):
			shardMetricsManager.ctx.Log().Info(fmt.Sprintf("Waiting for result timeout, NodeNumber: %d, TimeOut: %d seconds",
				nodeNumber, SYNC_RESULT_TIMEOUT_SECONDS))
		}
	}
}

func (shardMetricsManager *ShardMetricsManager) getNodeShardInfoFromShardList(groupName string, node Node) GroupShardMetrics {
	address := node.Host + ":" + strconv.Itoa(int(node.Port))
	nodeHash := GetNodeHash(groupName, node.NodeId)
	shardListInfo := GroupShardMetrics{
		nodeInfo:      make(map[uint64]NodeBasicInfo),
		nodeShardInfo: make(map[uint64]map[string]ShardInfo),
	}
	shardList := make(map[string]ShardInfo)
	url := shardMetricsManager.getShardListUrl(address)
	shardMetricsManager.ctx.Log().Debug(fmt.Sprint("start pull shard list, url:", url))
	resp, err := http.Get(url)
	if err != nil {
		shardMetricsManager.ctx.Log().Error(fmt.Sprint("get shard list fail, url:", url, " err:", err.Error()))
		return shardListInfo

	}
	defer resp.Body.Close()
	shardListJson := ShardListJson{}
	response, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		shardMetricsManager.ctx.Log().Error(fmt.Sprint("parse shard list result fail, url:", url, " err:", err.Error()))
		return shardListInfo

	}
	err = json.Unmarshal(response, &shardListJson)
	if err != nil {
		shardMetricsManager.ctx.Log().Error(fmt.Sprint("Unmarshal response failed, url:", url, " err:", err.Error()))
		return shardListInfo

	}
	for _, shardInfo := range shardListJson.Data {
		shardBasicInfo := ShardInfo{
			ShardId:   shardInfo.ShardId,
			ShardHash: GetShardHash(nodeHash, shardInfo.ShardId),
		}
		if shardInfo.Status == AvailableString {
			shardBasicInfo.ServiceState = Available
		} else {
			shardBasicInfo.ServiceState = Unavailable
		}
		if shardInfo.Role == "leader" {
			shardBasicInfo.Role = Leader
		} else {
			shardBasicInfo.Role = Follower
		}
		sort.Sort(shardInfo.Partitions)
		var versionHash uint64 = 0
		var seqNumber uint64 = 0
		for _, partitionInfo := range shardInfo.Partitions {
			versionHash = cityhash.CityHash64WithSeed([]byte(partitionInfo.BaseVersion), uint32(len(partitionInfo.BaseVersion)), versionHash)
			seqNumber += partitionInfo.DbInfo.SeqNo
		}
		shardBasicInfo.SeqNo = seqNumber
		shardBasicInfo.BaseVersionHash = versionHash
		shardBasicInfo.Partitions = shardInfo.Partitions
		shardList[shardBasicInfo.ShardHash] = shardBasicInfo
		shardListInfo.nodeShardInfo[nodeHash] = shardList
	}
	return shardListInfo
}

func (shardMetricsManager *ShardMetricsManager) getShardMetricsFromServerStatus(groupName string, node Node,
	nodeHash uint64, shardList map[string]ShardInfo) (bool, map[string]ShardInfo) {
	isNodeAvailable := false
	address := node.Host + ":" + strconv.Itoa(int(node.Port))

	shardMetricsInfo := make(map[string]ShardInfo)
	url := shardMetricsManager.getServerStatusUrl(address)
	shardMetricsManager.ctx.Log().Debug(fmt.Sprint("start pull server status, url:", url))
	resp, err := http.Get(url)
	if err != nil {
		shardMetricsManager.ctx.Log().Error(fmt.Sprint("get server status fail, url:", url, " err:", err.Error()))
		return isNodeAvailable, shardMetricsInfo

	}
	defer resp.Body.Close()
	serverStatusJson := ServerStatus{}
	response, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		shardMetricsManager.ctx.Log().Error(fmt.Sprint("parse server status result fail, url:", url, " err:", err.Error()))
		return isNodeAvailable, shardMetricsInfo

	}
	err = json.Unmarshal(response, &serverStatusJson)
	if err != nil {
		shardMetricsManager.ctx.Log().Error(fmt.Sprint("Unmarshal response failed, url:", url, " err:", err.Error()))
		return isNodeAvailable, shardMetricsInfo

	}

	if serverStatusJson.ServiceBase.ServerStatus == AvailableString {
		isNodeAvailable = true
	}

	for _, shardInfo := range shardList {
		var metricsInfo ShardInfo
		metric := make(map[string]float64)
		shardHash := GetShardHash(nodeHash, shardInfo.ShardId)
		for _, metricName := range metricsType {
			key := shardMetricsManager.getMetricKey(node.NodeId, string(metricName), shardInfo.ShardId)
			if val, ok := serverStatusJson.Shard.Stat[key]; ok {
				if val.Value != 0 {
					metric[string(metricName)] = val.Value
				}
			}
		}
		metricsInfo = ShardInfo{
			ShardId:         shardInfo.ShardId,
			ShardHash:       shardInfo.ShardHash,
			Role:            shardInfo.Role,
			ServiceState:    shardInfo.ServiceState,
			SeqNo:           shardInfo.SeqNo,
			BaseVersionHash: shardInfo.BaseVersionHash,
			Metrics:         metric,
			Partitions:      shardInfo.Partitions,
		}
		shardMetricsInfo[shardHash] = metricsInfo
	}
	return isNodeAvailable, shardMetricsInfo
}

func GetNodeHash(group string, nodeId uint32) uint64 {
	return cityhash.CityHash64WithSeed([]byte(group), uint32(len(group)), uint64(nodeId))
}

func GetShardHash(nodeHash uint64, shardId uint32) string {
	shardIdStr := strconv.Itoa(int(shardId))
	return strconv.FormatUint(cityhash.CityHash64WithSeed([]byte(shardIdStr), uint32(len(shardIdStr)), nodeHash), 10)
}

func GetPartitionHash(databaseName string, tableName string, partitionId uint32) uint64 {
	key := cityhash.CityHash64WithSeed([]byte(databaseName), uint32(len(databaseName)), uint64(partitionId))
	return cityhash.CityHash64WithSeed([]byte(tableName), uint32(len(tableName)), key)
}

func GetTableHash(databaseName string, tableName string) uint64 {
	key := cityhash.CityHash64WithSeed([]byte(databaseName), uint32(len(databaseName)), 0)
	return cityhash.CityHash64WithSeed([]byte(tableName), uint32(len(tableName)), key)
}

func (shardMetricsManager *ShardMetricsManager) GetShardNum(dc string) uint32 {
	var params params.ClusterListParams
	var shardNumber uint32
	cluster := NewClusterModel(shardMetricsManager.ctx)
	clusterList, _ := cluster.List(params, false)
	for _, clusterInfo := range clusterList {
		for _, datacenter := range clusterInfo.Dcs {
			if datacenter.Name == dc {
				shardNumber = uint32(datacenter.ShardNumber)
				break
			}
		}
		// 如果dc没有设置shardNumber，则用默认的
		if shardNumber == 0 {
			// 当前只有一个cluster，所以本处暂不做判断，直接拿来使用
			shardNumber = clusterInfo.ShardTotal
			break
		}
	}
	return shardNumber
}

func (shardMetricsManager *ShardMetricsManager) GetPartitionNumAndDc(databaseName string, tableName string) (uint32, string) {
	var paramTable params.TableListParams
	var partitionNumber uint32
	var dc string
	table := NewTableModel(shardMetricsManager.ctx)
	tableList, _ := table.List(paramTable, false)
	for _, tableInfo := range tableList {
		if tableInfo.Database.Name == databaseName && tableInfo.Name == tableName {
			partitionNumber = tableInfo.PartitionNumber
			dc = tableInfo.Dc.Name
			break
		}
	}
	return partitionNumber, dc
}

func (shardMetricsManager *ShardMetricsManager) GetTableShardList(databaseName string, tableName string) []int64 {
	partitionNumber, dc := shardMetricsManager.GetPartitionNumAndDc(databaseName, tableName)
	shardNumber := shardMetricsManager.GetShardNum(dc)
	shardList := make([]int64, 0)
	uniqueShardList := make(map[int64]bool, 0)

	for partitionId := uint32(0); partitionId < partitionNumber; partitionId++ {
		partition := lib.NewPartition(databaseName, tableName, partitionId)
		shardId := lib.GetShardIdByShardNumber(partition, shardNumber)
		if _, ok := uniqueShardList[shardId]; !ok {
			uniqueShardList[shardId] = true
			shardList = append(shardList, int64(shardId))
		}
	}

	return shardList
}

func (shardMetricsManager *ShardMetricsManager) GetTableShardSizeAndKps(databaseName string,
	tableName string) (shardSize, shardKps map[int64]uint64) {
	shardSize = make(map[int64]uint64, 0)
	shardKps = make(map[int64]uint64, 0)
	tableShardList := shardMetricsManager.GetTableShardList(databaseName, tableName)
	shardMetricsManager.Lock()
	defer shardMetricsManager.Unlock()
	for _, groupMetrics := range shardMetricsManager.shardMetrics {
		for nodeHash, nodeMetrics := range groupMetrics.nodeShardInfo {
			for _, shardId := range tableShardList {
				shardHash := GetShardHash(nodeHash, uint32(shardId))
				for _, partitionMetrics := range nodeMetrics[shardHash].Partitions {
					if partitionMetrics.DatabaseName == databaseName &&
						partitionMetrics.TableName == tableName {
						if partitionMetrics.Role == "leader" {
							shardSize[shardId] += partitionMetrics.Size
						}
						shardKps[shardId] += partitionMetrics.ReadKps
					}
				}
			}
		}
	}
	return
}

func (shardMetricsManager *ShardMetricsManager) IsNodeAvailable(groupName string, nodeId uint32) bool {
	nodeHash := GetNodeHash(groupName, nodeId)
	shardMetricsManager.Lock()
	defer shardMetricsManager.Unlock()
	groupMetrics, ok := shardMetricsManager.shardMetrics[groupName]
	if !ok {
		return false
	}
	if val, ok := groupMetrics.nodeInfo[nodeHash]; ok {
		return val.IsAvailable
	}
	return false
}

func (shardMetricsManager *ShardMetricsManager) GetNodeMetricValue(groupName string, nodeId uint32,
	metricName MetricNameString) float64 {
	sumMetricValue := float64(0)
	nodeHash := GetNodeHash(groupName, nodeId)
	shardMetricsManager.Lock()
	defer shardMetricsManager.Unlock()
	groupMetrics, ok := shardMetricsManager.shardMetrics[groupName]
	if !ok {
		return sumMetricValue
	}
	if nodeMetrics, ok := groupMetrics.nodeShardInfo[nodeHash]; ok {
		for _, shardMetrics := range nodeMetrics {
			if metricValue, ok := shardMetrics.Metrics[string(metricName)]; ok {
				sumMetricValue += metricValue
			}
		}
	}
	return sumMetricValue
}

func (shardMetricsManager *ShardMetricsManager) getMetricKey(nodeId uint32, metricName string, shardId uint32) string {
	metricKey := "property=rocksdb." + metricName + ",shard_id=" +
		strconv.Itoa(int(shardId)) + ",node_id=" + strconv.Itoa(int(nodeId))
	return metricKey
}

func (shardMetricsManager *ShardMetricsManager) getShardListUrl(address string) string {
	url := "http://" + address + "/shard/list"
	return url
}

func (shardMetricsManager *ShardMetricsManager) getServerStatusUrl(address string) string {
	url := "http://" + address + "/server/status"
	return url
}

func (shardMetricsManager *ShardMetricsManager) getShardIdByDbAndTb(databaseName string, tableName string,
	partitionId uint32, shardNumber uint32) uint32 {
	partitionHash := GetPartitionHash(databaseName, tableName, partitionId)
	shardId := uint64(math.Abs(float64(partitionHash))) % uint64(shardNumber)
	return uint32(shardId)
}

func (shardMetricsManager *ShardMetricsManager) getShardMetricsFromCacheById(shardId uint32) ShardInfo {
	var result ShardInfo
	for _, groupMetrics := range shardMetricsManager.shardMetrics {
		for _, nodeMetrics := range groupMetrics.nodeShardInfo {
			for _, shardMetrics := range nodeMetrics {
				if shardMetrics.ShardId == shardId {
					return shardMetrics
				}
			}
		}
	}
	return result
}

func (shardMetricsManager *ShardMetricsManager) checkShardExist(shardId uint32, shardInfo []ShardInfo) bool {
	var exist bool = false
	for _, info := range shardInfo {
		if info.ShardId == shardId {
			exist = true
			break
		}
	}

	return exist
}

func (shardMetricsManager *ShardMetricsManager) GetNodeShardInfo(groupName string, nodeId uint32) (NodeBasicInfo, map[string]ShardInfo) {
	var nodeInfo NodeBasicInfo
	nodeShardInfo := make(map[string]ShardInfo)
	shardMetricsManager.Lock()
	nodeHash := GetNodeHash(groupName, nodeId)
	nodeInfoStr, _ := json.Marshal(shardMetricsManager.shardMetrics[groupName].nodeInfo[nodeHash])
	nodeShardInfoStr, _ := json.Marshal(shardMetricsManager.shardMetrics[groupName].nodeShardInfo[nodeHash])
	shardMetricsManager.Unlock()
	_ = json.Unmarshal(nodeInfoStr, &nodeInfo)
	_ = json.Unmarshal(nodeShardInfoStr, &nodeShardInfo)

	return nodeInfo, nodeShardInfo
}

func (shardMetricsManager *ShardMetricsManager) getShardListByDbAndTb(databaseName string, tableName string) []ShardInfo {
	result := make([]ShardInfo, 0)
	partitionNumer, dc := shardMetricsManager.GetPartitionNumAndDc(databaseName, tableName)
	shardNumber := shardMetricsManager.GetShardNum(dc)
	for partitionId := 0; partitionId <= int(partitionNumer); partitionId++ {
		shardId := shardMetricsManager.getShardIdByDbAndTb(databaseName, tableName, uint32(partitionId), shardNumber)
		shardMetrics := shardMetricsManager.getShardMetricsFromCacheById(shardId)
		isExist := shardMetricsManager.checkShardExist(shardId, result)
		if !isExist {
			result = append(result, shardMetrics)
		}
	}
	return result
}

func (shardMetricsManager *ShardMetricsManager) GetSpecShardNum(shardId uint32, shardRole ShardRole) uint32 {
	var num uint32 = 0
	for _, groupMetrics := range shardMetricsManager.shardMetrics {
		for _, nodeMetrics := range groupMetrics.nodeShardInfo {
			for _, shardMetrics := range nodeMetrics {
				if shardMetrics.ShardId == shardId && shardMetrics.Role == shardRole {
					num++
				}
			}
		}
	}
	return num
}
