package service

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"strconv"
	"time"

	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
)

type ConsulApiReturnData struct {
	LockIndex   int    `json:"LockIndex"`
	Flags       int    `json:"Flags"`
	Value       string `json:"Value"`
	CreateIndex int64  `json:"CreateIndex"`
	ModifyIndex int64  `json:"ModifyIndex"`
}

const (
	ConsulDatabaseTableSchemaKey    string = "database_table_schema_data"
	ConsulTableConfigListKey        string = "table_config_list_data"
	ConsulTrafficRestrictionKey     string = "traffic_restriction_data"
	ConsulProxyTableConfigSchemaKey string = "table_config"
	ConsulNodeConfigListKey         string = "node_config_list_data"
	ConsulClusterInfoKey            string = "cluster_info_data"
	ConsulRocksdbNodeConfigsKey     string = "rocksdb_node_configs_data"
)

type LaserConfigType uint32

const (
	ClusterInfoData LaserConfigType = iota + 1
	TableInfoData
)

var LaserConfigKeyToType = map[string]LaserConfigType{
	ConsulDatabaseTableSchemaKey: TableInfoData,
	ConsulTableConfigListKey:     TableInfoData,
	ConsulTrafficRestrictionKey:  TableInfoData,
	ConsulClusterInfoKey:         ClusterInfoData,
	ConsulNodeConfigListKey:      ClusterInfoData,
	ConsulRocksdbNodeConfigsKey:  ClusterInfoData,
}

type TableInfo struct {
	Engine          string   `json:"Engine"`
	TableName       string   `json:"TableName"`
	PartitionNumber uint32   `json:"PartitionNumber"`
	Ttl             uint64   `json:"Ttl"`
	Loaders         []string `json:"Loaders"`
	ConfigName      string   `json:"ConfigName"`
	BindEdgeNodes   []string `json:"BindEdgeNodes"`
	EdgeFlowRatio   uint32   `json:"EdgeFlowRatio"`
	Dc              string   `json:"Dc"`
	DistDc          string   `json:"DistDc"`
}

type DatabaseInfo struct {
	DatabaseName string      `json:"DatabaseName"`
	Tables       []TableInfo `json:"Tables"`
}

type TableConfigInfo struct {
	DbOptions    map[string]string `json:"DbOptions"`
	TableOptions map[string]string `json:"TableOptions"`
	CfOptions    map[string]string `json:"CfOptions"`
	Version      uint32            `json:"Version"`
}

type TableConfigListInfo struct {
	TableConfigList map[string]TableConfigInfo `json:"TableConfigList"`
}

type TrafficRestrictionLimitItem struct {
	Type  uint32 `json:"Type"` // 0-KPS 1-QPS
	Limit uint32 `json:"Limit"`
}

type TableTrafficRestrictionInfo struct {
	TableName               string                                 `json:"TableName"`
	DenyAll                 bool                                   `json:"DenyAll"`
	SingleOperationLimits   map[string]uint32                      `json:"SingleOperationLimits"`
	MultipleOperationLimits map[string]TrafficRestrictionLimitItem `json:"MultipleOperationLimits"`
}

type DatabaseTrafficRestrictionInfo struct {
	DatabaseName string                        `json:"DatabaseName"`
	Tables       []TableTrafficRestrictionInfo `json:"Tables"`
}

type TrafficRestrictionInfo struct {
	Infos []DatabaseTrafficRestrictionInfo `json:"Infos"`
}

type ProxyConfig struct {
	DefaultReadTimeout  uint32                       `json:"LaserClientReadTimeout"`
	DefaultWriteTimeout uint32                       `json:"LaserClientWriteTimeout"`
	DefaultAllowedFlow  uint32                       `json:"LaserClientAllowedFlow"`
	ProxyTableConfig    ProxyDatabaseTableConfigList `json:"ProxyTableConfig"`
}

type ProxyDatabaseTableConfigList struct {
	ProxyDbTableConfigList map[string]ProxyConfigInfo `json:"ProxyDbTableConfigList"`
}

type ProxyConfigInfo struct {
	ProxyDbTableConfig map[string]ProxyTableInfo `json:"ProxyDbTableConfig"`
}

type ProxyTableInfo struct {
	ReadTimeout  uint32 `json:"TableReadTimeout"`
	WriteTimeout uint32 `json:"TableWriteTimeout"`
	AllowedFlow  uint32 `json:"TableAllowedFlow"`
}

type NodeConfigListInfo struct {
	NodeConfigList map[string]NodeConfigInfo `json:"NodeConfigList"`
}

type NodeRateLimitInfo struct {
	BeginHour       uint32 `json:"BeginHour"`
	EndHour         uint32 `json:"EndHour"`
	RateBytesPerSec int64  `json:"RateBytesPerSec"`
}

type NodeConfigInfo struct {
	BlockCacheSizeGb    uint32              `json:"BlockCacheSizeGb"`
	WriteBufferSizeGb   uint32              `json:"WriteBufferSizeGb"`
	NumShardBits        int32               `json:"NumShardBits"`
	HighPriPoolRatio    json.Number         `json:"HighPriPoolRatio"`
	StrictCapacityLimit bool                `json:"StrictCapacityLimit"`
	RateLimitStrategy   []NodeRateLimitInfo `json:"RateLimitStrategy"`
}

type RocksdbNodeConfigsInfo struct {
	RocksdbNodeConfigs map[string]string `json:"RocksdbNodeConfigs"`
}

type ConfigDataGeneratorFunc func() ([]byte, *common.Status)

type ConsulModel struct {
	ctx                      *context.Context
	consulKeyToGeneratorFunc map[string]ConfigDataGeneratorFunc
}

func NewConsulModel(ctx *context.Context) *ConsulModel {
	instance := &ConsulModel{
		ctx: ctx,
	}
	funcMap := make(map[string]ConfigDataGeneratorFunc)
	funcMap[ConsulDatabaseTableSchemaKey] = instance.generateDatabaseTableSchemaJson
	funcMap[ConsulTableConfigListKey] = instance.generateTableConfigListJson
	funcMap[ConsulTrafficRestrictionKey] = instance.generateTrafficRestrictionJson
	funcMap[ConsulNodeConfigListKey] = instance.generateNodeConfigListJson
	funcMap[ConsulClusterInfoKey] = instance.generateClusterInfo
	funcMap[ConsulRocksdbNodeConfigsKey] = instance.generateRocksdbNodeConfigsJson
	instance.consulKeyToGeneratorFunc = funcMap
	return instance
}

func (consul *ConsulModel) SynchronizeDatabaseTableSchemaToConsul() *common.Status {
	schemaData, status := consul.generateDatabaseTableSchemaJson()
	if status.Code() != common.OK {
		return status
	}

	tableConfigListData, status := consul.generateTableConfigListJson()
	if status.Code() != common.OK {
		return status
	}

	trafficRestrictionData, status := consul.generateTrafficRestrictionJson()
	if status.Code() != common.OK {
		return status
	}

	serviceName := consul.ctx.GetConfig().ServiceName()
	if status := consul.consulSetLaserConfigsKV(serviceName,
		ConsulTableConfigListKey, tableConfigListData); status.Code() != common.OK {
		return status
	}

	if status := consul.consulSetLaserConfigsKV(serviceName,
		ConsulTrafficRestrictionKey, trafficRestrictionData); status.Code() != common.OK {
		return status
	}

	return consul.consulSetLaserConfigsKV(serviceName, ConsulDatabaseTableSchemaKey, schemaData)
}

func (consul *ConsulModel) SynchronizeClusterInfoToConsul() *common.Status {
	schemaData, status := consul.generateNodeConfigListJson()
	if !status.Ok() {
		return status
	}

	rocksdbNodeConfigData, status := consul.generateRocksdbNodeConfigsJson()
	if !status.Ok() {
		return status
	}

	clusterInfo, status := consul.generateClusterInfo()
	if !status.Ok() {
		return status
	}

	serviceName := consul.ctx.GetConfig().ServiceName()
	if status = consul.consulSetLaserConfigsKV(serviceName,
		ConsulNodeConfigListKey, schemaData); !status.Ok() {
		return status
	}
	if status := consul.consulSetLaserConfigsKV(serviceName,
		ConsulRocksdbNodeConfigsKey, rocksdbNodeConfigData); !status.Ok() {
		return status
	}

	return consul.consulSetLaserConfigsKV(serviceName, ConsulClusterInfoKey, clusterInfo)
}

func (consul *ConsulModel) SynchronizeProxyTableConfigSchemaToConsul() *common.Status {
	schemaData, status := consul.generateProxyTableConfigSchemaJson()
	if status.Code() != common.OK {
		return status
	}
	proxyServiceName := consul.ctx.GetConfig().ProxyServiceName()
	return consul.consulSetLaserConfigsKV(proxyServiceName, ConsulProxyTableConfigSchemaKey, schemaData)
}

func (consul *ConsulModel) consulUpdateConfigVersion(serviceName string) *common.Status {
	url := "http://" + consul.ctx.GetConfig().ConsulAddress() +
		"/v1/kv/ads-core/services/laser_" + serviceName + "/config/version?token=" + consul.ctx.GetConfig().ConsulToken()
	now := time.Now().UnixNano() / 1e6
	request, err := http.NewRequest("PUT", url, bytes.NewBuffer([]byte(fmt.Sprintf("%d", now))))
	if err != nil {
		return common.StatusWithError(err)
	}
	request.Header.Add("Content-Type", "application/json")
	response, err := http.DefaultClient.Do(request)
	if err != nil {
		return common.StatusWithError(err)
	}
	defer response.Body.Close()
	if _, err = ioutil.ReadAll(response.Body); err != nil {
		return common.StatusWithError(err)
	}
	if response.StatusCode != http.StatusOK {
		return common.StatusWithMessage(common.ConsulPutKVFailed, response.Status)
	}
	return common.StatusOk()
}

func (consul *ConsulModel) consulSetLaserConfigsKV(serviceName string, key string, value []byte) *common.Status {
	url := "http://" + consul.ctx.GetConfig().ConsulAddress() +
		"/v1/kv/ads-core/services/laser_" + serviceName + "/config/configs/" +
		key + "?token=" + consul.ctx.GetConfig().ConsulToken()
	request, err := http.NewRequest("PUT", url, bytes.NewBuffer(value))
	if err != nil {
		return common.StatusWithError(err)
	}
	request.Header.Add("Content-Type", "application/json")
	response, err := http.DefaultClient.Do(request)
	if err != nil {
		return common.StatusWithError(err)
	}

	defer response.Body.Close()
	if _, err = ioutil.ReadAll(response.Body); err != nil {
		return common.StatusWithError(err)
	}
	if response.StatusCode != http.StatusOK {
		return common.StatusWithMessage(common.ConsulPutKVFailed, response.Status)
	}
	return consul.consulUpdateConfigVersion(serviceName)
}

func (consul *ConsulModel) consulGetLaserConfigsKV(key string) ([]byte, *common.Status) {
	serviceName := consul.ctx.GetConfig().ServiceName()
	url := consul.getConsulUrl(serviceName, key)
	response, err := http.Get(url)
	if err != nil {
		return nil, common.StatusWithError(err)
	}
	defer response.Body.Close()
	data, err := ioutil.ReadAll(response.Body)

	var consulReturns []ConsulApiReturnData
	err = json.Unmarshal(data, &consulReturns)
	if err != nil {
		return nil, common.StatusWithError(err)
	}
	if len(consulReturns) < 1 {
		return nil, common.StatusError(common.ConsulGetKVFailed)
	}
	value, err := base64.StdEncoding.DecodeString(consulReturns[0].Value)

	return value, common.StatusOk()
}

func (consul *ConsulModel) getConsulUrl(serviceName string, key string) string {
	url := "http://" + consul.ctx.GetConfig().ConsulAddress() +
		"/v1/kv/ads-core/services/laser_" + serviceName + "/config/configs/" +
		key + "?token=" + consul.ctx.GetConfig().ConsulToken()
	return url
}

func (consul *ConsulModel) generateDatabaseTableSchemaJson() ([]byte, *common.Status) {
	dbInfos := make([]DatabaseInfo, 0)
	dbMod := NewLaserDatabaseModel(consul.ctx)
	tableMod := NewTableModel(consul.ctx)
	dbList, _ := dbMod.List(params.DatabaseListParams{}, false)
	for _, dbItem := range dbList {
		dbInfo := DatabaseInfo{
			DatabaseName: dbItem.Name,
			Tables:       make([]TableInfo, 0),
		}
		dbId := dbItem.Id
		tableList, _ := tableMod.ListDetail(params.TableListParams{DatabaseId: dbId}, false)
		for _, tableItem := range tableList {
			if *tableItem.Status == TableDisabled {
				continue
			}
			if tableItem.Config.Name == "" {
				tableItem.Config.Name = DefaultConfigName
			}

			groupModel := NewGroupModel(consul.ctx)
			bindEdgeNodes := make([]string, 0)
			for _, node := range tableItem.BindEdgeNodes {
				groupListParams := params.GroupListParams{
					GroupId: node.GroupId,
				}
				groupInfo, totalSize := groupModel.List(groupListParams, true)
				if totalSize != 1 {
					continue
				}
				uniqueId := groupInfo[0].Name + "#" + strconv.FormatInt(int64(node.NodeId), 10)
				bindEdgeNodes = append(bindEdgeNodes, uniqueId)
			}

			tableInfo := TableInfo{
				Engine:          "rocksdb",
				TableName:       tableItem.Name,
				PartitionNumber: tableItem.PartitionNumber,
				Ttl:             *tableItem.Ttl,
				Loaders:         make([]string, 0),
				ConfigName:      tableItem.Config.Name,
				BindEdgeNodes:   bindEdgeNodes,
				EdgeFlowRatio:   *tableItem.EdgeFlowRatio,
				Dc:              tableItem.Dc.Name,
				DistDc:          tableItem.DistDc.Name,
			}
			dbInfo.Tables = append(dbInfo.Tables, tableInfo)
		}
		dbInfos = append(dbInfos, dbInfo)
	}

	config_string, err := json.Marshal(dbInfos)
	if err != nil {
		return config_string, common.StatusWithError(err)
	}

	return config_string, common.StatusOk()
}

func (consul *ConsulModel) generateTableConfigListJson() ([]byte, *common.Status) {
	configListInfo := TableConfigListInfo{TableConfigList: make(map[string]TableConfigInfo)}
	db := NewTableConfigModel(consul.ctx)
	configList, _ := db.ListWithDetailItems(params.TableConfigListParams{}, false)
	for _, tableConfig := range configList {
		configInfo := TableConfigInfo{
			DbOptions:    make(map[string]string),
			TableOptions: make(map[string]string),
			CfOptions:    make(map[string]string),
		}
		for _, configItem := range tableConfig.ConfigItems {
			if configItem.Type == TypeDbOptions {
				configInfo.DbOptions[configItem.Name] = configItem.Value
			} else if configItem.Type == TypeTableOptions {
				configInfo.TableOptions[configItem.Name] = configItem.Value
			} else {
				configInfo.CfOptions[configItem.Name] = configItem.Value
			}
		}
		configInfo.Version = tableConfig.Version
		configListInfo.TableConfigList[tableConfig.Name] = configInfo
	}

	tableConfigListString, err := json.Marshal(configListInfo)
	if err != nil {
		return tableConfigListString, common.StatusWithError(err)
	}

	return tableConfigListString, common.StatusOk()
}

func (consul *ConsulModel) generateProxyTableConfigSchemaJson() ([]byte, *common.Status) {
	var configListInfo ProxyConfig
	configInfoList := ProxyDatabaseTableConfigList{
		ProxyDbTableConfigList: make(map[string]ProxyConfigInfo),
	}
	configListInfo.ProxyTableConfig = configInfoList
	dbMod := NewLaserDatabaseModel(consul.ctx)
	configListMod := NewProxyTableConfigModel(consul.ctx)
	defaultConfig, _ := configListMod.List(params.ProxyTableConfigListParams{DatabaseName: defaultDbName}, true)
	if cap(defaultConfig) == 0 {
		configListInfo.DefaultReadTimeout = 10
		configListInfo.DefaultWriteTimeout = 10
		configListInfo.DefaultAllowedFlow = 100
	} else {
		for _, value := range defaultConfig {
			configListInfo.DefaultReadTimeout = value.ReadTimeout
			configListInfo.DefaultWriteTimeout = value.WriteTimeout
			configListInfo.DefaultAllowedFlow = value.AllowedFlow
		}
	}

	dbList, _ := dbMod.List(params.DatabaseListParams{}, false)
	for _, dbItem := range dbList {
		tableListConfig, _ := configListMod.List(params.ProxyTableConfigListParams{DatabaseName: dbItem.Name}, true)
		if cap(tableListConfig) == 0 {
			continue
		}
		configInfo := ProxyConfigInfo{
			ProxyDbTableConfig: make(map[string]ProxyTableInfo),
		}
		for _, configItem := range tableListConfig {
			var specificConfig ProxyTableInfo
			specificConfig.ReadTimeout = configItem.ReadTimeout
			specificConfig.WriteTimeout = configItem.WriteTimeout
			specificConfig.AllowedFlow = configItem.AllowedFlow
			configInfo.ProxyDbTableConfig[configItem.TableName] = specificConfig
		}
		configListInfo.ProxyTableConfig.ProxyDbTableConfigList[dbItem.Name] = configInfo
	}
	tableConfigListString, err := json.Marshal(configListInfo)
	if err != nil {
		return tableConfigListString, common.StatusWithError(err)
	}
	return tableConfigListString, common.StatusOk()
}

func (consul *ConsulModel) toNumber(f float64) json.Number {
	var s string
	if f == float64(int64(f)) {
		s = fmt.Sprintf("%.1f", f) // 1 decimal if integer
	} else {
		s = fmt.Sprint(f)
	}

	return json.Number(s)
}

func (consul *ConsulModel) generateNodeConfigListJson() ([]byte, *common.Status) {
	configInfo := NodeConfigListInfo{
		NodeConfigList: make(map[string]NodeConfigInfo),
	}
	configListMod := NewNodeConfigModel(consul.ctx)
	NodeConfigList, _ := configListMod.List(params.NodeConfigListParams{}, true)

	for _, configItem := range NodeConfigList {
		var capacityLimit bool
		if configItem.StrictCapacityLimit == 0 {
			capacityLimit = false
		} else {
			capacityLimit = true
		}

		nodeRateLimitInfos := make([]NodeRateLimitInfo, 0)
		for _, rateLimit := range configItem.RateLimitStrategy {
			nodeRateLimitInfo := NodeRateLimitInfo{
				BeginHour:       *rateLimit.BeginHour,
				EndHour:         rateLimit.EndHour,
				RateBytesPerSec: rateLimit.RateBytesPerSec,
			}
			nodeRateLimitInfos = append(nodeRateLimitInfos, nodeRateLimitInfo)
		}
		specificConfig := NodeConfigInfo{
			BlockCacheSizeGb:    configItem.BlockCacheSizeGb,
			WriteBufferSizeGb:   configItem.WriteBufferSizeGb,
			NumShardBits:        configItem.NumShardBits,
			HighPriPoolRatio:    consul.toNumber(*configItem.HighPriPoolRatio),
			StrictCapacityLimit: capacityLimit,
			RateLimitStrategy:   nodeRateLimitInfos,
		}
		configInfo.NodeConfigList[configItem.Name] = specificConfig
	}

	NodeConfigListString, err := json.Marshal(configInfo)
	if err != nil {
		return NodeConfigListString, common.StatusWithError(err)
	}
	return NodeConfigListString, common.StatusOk()
}

func (consul *ConsulModel) generateTrafficRestrictionJson() ([]byte, *common.Status) {
	trafficRestrictions := make([]DatabaseTrafficRestrictionInfo, 0)
	dbMod := NewLaserDatabaseModel(consul.ctx)
	tableMod := NewTableModel(consul.ctx)
	dbList, _ := dbMod.List(params.DatabaseListParams{}, false)
	for _, dbInfo := range dbList {
		databaseTrafficRestrictionInfo := DatabaseTrafficRestrictionInfo{
			DatabaseName: dbInfo.Name,
			Tables:       make([]TableTrafficRestrictionInfo, 0),
		}
		tableList, _ := tableMod.ListDetail(params.TableListParams{DatabaseId: dbInfo.Id}, false)
		for _, tableInfo := range tableList {
			if *tableInfo.DenyAll == 0 && len(tableInfo.TrafficLimits) == 0 {
				continue
			}
			tableTrafficRestriction := TableTrafficRestrictionInfo{
				TableName:               tableInfo.Name,
				DenyAll:                 *tableInfo.DenyAll != 0,
				SingleOperationLimits:   make(map[string]uint32, 0),
				MultipleOperationLimits: make(map[string]TrafficRestrictionLimitItem, 0),
			}

			for _, limit := range tableInfo.TrafficLimits {
				if limit.OperationType == SingleOperation {
					tableTrafficRestriction.SingleOperationLimits[limit.Name] = limit.LimitValue
				} else if limit.OperationType == MultipleOperation {
					restrictionItem := TrafficRestrictionLimitItem{
						Type:  *limit.LimitType,
						Limit: limit.LimitValue,
					}
					tableTrafficRestriction.MultipleOperationLimits[limit.Name] = restrictionItem
				}
			}
			databaseTrafficRestrictionInfo.Tables = append(databaseTrafficRestrictionInfo.Tables, tableTrafficRestriction)
		}
		trafficRestrictions = append(trafficRestrictions, databaseTrafficRestrictionInfo)
	}

	trafficRestrictionJsonString, err := json.Marshal(trafficRestrictions)
	if err != nil {
		return trafficRestrictionJsonString, common.StatusWithError(err)
	}

	return trafficRestrictionJsonString, common.StatusOk()
}

func (consul *ConsulModel) generateClusterInfo() ([]byte, *common.Status) {
	nodeModel := GetNodeModel(consul.ctx)
	_, clusterInfo := nodeModel.GetClusterInfo()
	shardManagerModel := GetShardModel(consul.ctx, clusterInfo)
	clusterInfoStr, status := shardManagerModel.GetCurrentVersionData()
	return []byte(clusterInfoStr), status
}

func (consul *ConsulModel) generateRocksdbNodeConfigsJson() ([]byte, *common.Status) {
	rocksdbNodeConfigsInfo := RocksdbNodeConfigsInfo{
		RocksdbNodeConfigs: make(map[string]string, 0),
	}
	nodeModel := GetNodeModel(consul.ctx)
	status, nodeGroups := nodeModel.ListNode()
	if !status.Ok() {
		return make([]byte, 0), status
	}

	for _, group := range nodeGroups {
		for _, node := range group.Nodes {
			nodeFlag := group.GroupName + "#" + strconv.FormatUint(uint64(node.NodeId), 10)
			if node.ConfigName == "" {
				node.ConfigName = DefaultConfigName
			}
			rocksdbNodeConfigsInfo.RocksdbNodeConfigs[nodeFlag] = node.ConfigName
		}
	}
	rocksdbNodeConfigsJson, err := json.Marshal(rocksdbNodeConfigsInfo)
	if err != nil {
		return rocksdbNodeConfigsJson, common.StatusWithError(err)
	}
	return rocksdbNodeConfigsJson, status
}
