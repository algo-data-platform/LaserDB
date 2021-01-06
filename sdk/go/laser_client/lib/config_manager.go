package lib

import (
	"encoding/json"
	"strings"
	"sync"

	"github.com/algo-data-platform/LaserDB/sdk/go/common/service_router"
	"github.com/zentures/cityhash"
)

const (
	LASER_CONFIG_DATABASE_TABLE_SCHEMA_DATA = "database_table_schema_data"
	LASER_CONFIG_CLUSTER_INFO               = "cluster_info_data"
	LASER_CONFIG_ROCKSDB_CONFIG             = "rocksdb_config_data"
)

type DatabaseSchema struct {
	DatabaseName string        `json:"DatabaseName"`
	Tables       []TableSchema `json:"Tables"`
}

type NodeShardList struct {
	NodeId            uint32   `json:"NodeId"`
	LeaderShardList   []uint32 `json:"LeaderShardList"`
	FollowerShardList []uint32 `json:"FollowerShardList"`
	IsEdgeNode        bool     `json:"IsEdgeNode"`
}

type DataCenter struct {
	Name        string `json:"Name"`
	ShardNumber uint32 `json:"ShardNumber"`
}

type ClusterGroup struct {
	GroupName string          `json:"GroupName"`
	Nodes     []NodeShardList `json:"Nodes"`
	Dc        string          `json:"Dc"`
}

type ClusterInfo struct {
	ClusterName string         `json:"ClusterName"`
	ShardNumber uint32         `json:"ShardNumber"`
	Dcs         []DataCenter   `json:"Dcs"`
	Groups      []ClusterGroup `json:"Groups"`
}

type TableSchema struct {
	DatabaseName    string   `json:"DatabaseName,omitempty"`
	TableName       string   `json:"TableName"`
	PartitionNumber int      `json:"PartitionNumber"`
	Ttl             uint64   `json:"Ttl"`
	BindEdgeNodes   []string `json:"BindEdgeNodes"`
	EdgeFlowRatio   int      `json:"EdgeFlowRatio"`
	Dc              string   `json:"Dc"`
	DistDc          string   `json:"DistDc"`
}

type ConfigManager struct {
	tableSchemasLock  sync.RWMutex
	clusterInfoLock   sync.RWMutex
	dcsLock           sync.RWMutex
	targetServiceName string
	router            *service_router.Router
	tableSchemas      map[uint64]TableSchema
	clusterInfo       ClusterInfo
	dcs               map[string]DataCenter
	// nodeShardLists    map[uint64]NodeShardList
}

func NewConfigManager(targetServiceName string, router *service_router.Router) *ConfigManager {
	configManager := &ConfigManager{
		targetServiceName: targetServiceName,
		router:            router,
	}
	// subscribe
	router.GetRegistry().SubscribeConfig(targetServiceName, configManager)
	router.GetOrCreateConfigPuller(targetServiceName)
	return configManager
}

func (cm *ConfigManager) getTableSchemaHash(databaseName string, tableName string) uint64 {
	key := cityhash.CityHash64WithSeed([]byte(databaseName), uint32(len(databaseName)), 0)
	return cityhash.CityHash64WithSeed([]byte(tableName), uint32(len(tableName)), key)
}

func (cm *ConfigManager) ConfigNotify(serviceName string, config service_router.ServiceConfig) {
	for k, v := range config.Configs {
		if strings.Contains(k, LASER_CONFIG_DATABASE_TABLE_SCHEMA_DATA) {
			var databaseSchemas []DatabaseSchema
			err := json.Unmarshal([]byte(v), &databaseSchemas)
			if err != nil {
				continue
			}
			tableSchemas := make(map[uint64]TableSchema, len(databaseSchemas))
			for _, databaseSchema := range databaseSchemas {
				for _, tableSchema := range databaseSchema.Tables {
					tableSchema.DatabaseName = databaseSchema.DatabaseName
					key := cm.getTableSchemaHash(tableSchema.DatabaseName, tableSchema.TableName)
					tableSchemas[key] = tableSchema
				}
			}

			cm.tableSchemasLock.Lock()
			cm.tableSchemas = tableSchemas
			cm.tableSchemasLock.Unlock()
		} else if strings.Contains(k, LASER_CONFIG_CLUSTER_INFO) {
			var clusterInfo ClusterInfo
			err := json.Unmarshal([]byte(v), &clusterInfo)
			if err != nil {
				continue
			}
			cm.clusterInfoLock.Lock()
			cm.clusterInfo = clusterInfo
			cm.clusterInfoLock.Unlock()

			dcs := make(map[string]DataCenter)
			for _, dc := range clusterInfo.Dcs {
				dcs[dc.Name] = dc
			}
			cm.dcsLock.Lock()
			cm.dcs = dcs
			cm.dcsLock.Unlock()
		}
	}
}

func (cm *ConfigManager) GetTableSchema(databaseName, tableName string) (TableSchema, bool) {
	key := cm.getTableSchemaHash(databaseName, tableName)
	cm.tableSchemasLock.RLock()
	defer cm.tableSchemasLock.RUnlock()
	res, ok := cm.tableSchemas[key]
	if ok {
		return res, true
	} else {
		return TableSchema{}, false
	}
}

func (cm *ConfigManager) GetShardNumber(dc string) uint32 {
	cm.dcsLock.RLock()
	defer cm.dcsLock.RUnlock()
	if dc, ok := cm.dcs[dc]; ok {
		return dc.ShardNumber
	}
	cm.clusterInfoLock.RLock()
	defer cm.clusterInfoLock.RUnlock()
	return cm.clusterInfo.ShardNumber
}
