package service_router

import (
	"sync"
	"time"

	"github.com/zentures/cityhash"
)

type RouterDb interface {
	ServiceSubscriber
	ConfigSubscriber

	GetConfigs() map[string]ServiceConfig
	GetServiceConfig(serviceName string) (map[string]string, bool)
	GetServiceRouterConfig(serviceName string) ServiceRouterConfig
	PickServers(serviceName string, serviceList ServerList) ServerList
	UpdateServers(serviceName string, serverList ServerList)
	UpdateConfig(serviceName string, config ServiceConfig)
	SelectServers(serviceName string, protocol string, routerId int64, shardType string, dc string) (ServerList, bool)
	BatchSelectServers(serviceName, protocol string, routerIds []int64, shardType string, dc string) map[int64]ServerList
}

// RouterDbImpl所提供的api返回的数据必须是副本
// golang没法控制引用的写权限
type RouterDbImpl struct {
	servicesLock      sync.RWMutex
	partitionKeyLock  sync.RWMutex
	configsLock       sync.RWMutex
	services          map[int64]ServerList
	lastPartitionKeys map[string][]int64
	configs           map[string]ServiceConfig
}

var (
	routerDbInstance RouterDb
	routerDbOnce     sync.Once
)

func GetServiceKey(serviceName string, protocol string, routerId int64, shardType string, dc string) int64 {
	key := cityhash.CityHash64WithSeed([]byte(dc), uint32(len(dc)), uint64(routerId))
	key = cityhash.CityHash64WithSeed([]byte(serviceName), uint32(len(serviceName)), key)
	key = cityhash.CityHash64WithSeed([]byte(protocol), uint32(len(protocol)), key)
	key = cityhash.CityHash64WithSeed([]byte(shardType), uint32(len(shardType)), key)
	return int64(key)
}

func GetRouterDb() RouterDb {
	routerDbOnce.Do(func() {
		routerDbInstance = &RouterDbImpl{
			services:          make(map[int64]ServerList),
			configs:           make(map[string]ServiceConfig),
			lastPartitionKeys: make(map[string][]int64),
		}
	})
	return routerDbInstance
}

func (rd *RouterDbImpl) GetConfigs() map[string]ServiceConfig {
	result := make(map[string]ServiceConfig, len(rd.configs))
	for k, v := range rd.configs {
		result[k] = v
	}
	return result
}

func (rd *RouterDbImpl) GetServiceConfig(serviceName string) (map[string]string, bool) {
	rd.configsLock.RLock()
	defer rd.configsLock.RUnlock()
	config, ok := rd.configs[serviceName]
	if ok && config.Configs != nil {
		var result = make(map[string]string, len(config.Configs))
		for k, v := range config.Configs {
			result[k] = v
		}
		return result, true
	} else {
		return nil, false
	}
}

func (rd *RouterDbImpl) GetServiceRouterConfig(serviceName string) ServiceRouterConfig {
	rd.configsLock.RLock()
	defer rd.configsLock.RUnlock()
	config, ok := rd.configs[serviceName]
	if ok {
		return config.Router
	} else {
		return DefaultServiceRouterConfig()
	}
}

func (rd *RouterDbImpl) PickServers(serviceName string, serviceList ServerList) ServerList {
	var result ServerList
	// current milliseconds
	now := time.Now().UnixNano() / 1e6
	routerConfig := rd.GetServiceRouterConfig(serviceName)
	for _, s := range serviceList {
		if uint64(now) > s.UpdateTime+uint64(routerConfig.TtlInMs) {
			continue
		}
		if s.Status != ServerStatus_AVAILABLE {
			continue
		}
		result = append(result, s)
	}
	return result
}

func (rd *RouterDbImpl) UpdateServers(serviceName string, serverList ServerList) {
	if len(serverList) == 0 {
		return
	}
	shardServers := make(map[int64]ServerList)
	partitionServers := make(map[int64]ServerList)
	pickedResult := rd.PickServers(serviceName, serverList)
	// 如果服务全挂了，那就赌一把，万一呢
	if len(pickedResult) == 0 {
		pickedResult = serverList
	}
	for _, sv := range pickedResult {
		if len(sv.Dc) == 0 {
			sv.Dc = DEFAULT_DC
		}
		if sv.IsEdgeNode {
			partitionList := sv.PartitionList
			for _, partitionHash := range partitionList {
				key := GetServiceKey(serviceName, sv.Protocol, partitionHash, ShardType_FOLLOWER, sv.Dc)
				partitionServers[key] = append(partitionServers[key], sv)
			}
			continue
		}
		availableShardList := sv.AvailableShardList
		followerAvailableShardList := sv.FollowerAvailableShardList
		if len(sv.ShardList) == 0 && len(sv.FollowerShardList) == 0 {
			// 按照不分区处理
			// 2^32 - 1
			availableShardList = append(availableShardList, uint32(UINT32MAX))
		}
		for _, as := range availableShardList {
			shardId := int64(as)
			key := GetServiceKey(serviceName, sv.Protocol, shardId, ShardType_LEADER, sv.Dc)
			shardServers[key] = append(shardServers[key], sv)
			key = GetServiceKey(serviceName, sv.Protocol, shardId, ShardType_ALL, sv.Dc)
			shardServers[key] = append(shardServers[key], sv)
		}
		for _, afs := range followerAvailableShardList {
			shardId := int64(afs)
			key := GetServiceKey(serviceName, sv.Protocol, shardId, ShardType_FOLLOWER, sv.Dc)
			shardServers[key] = append(shardServers[key], sv)
			key = GetServiceKey(serviceName, sv.Protocol, shardId, ShardType_ALL, sv.Dc)
			shardServers[key] = append(shardServers[key], sv)
		}
	}
	// clean partition ServerList
	rd.servicesLock.Lock()
	rd.partitionKeyLock.Lock()
	if _, ok := rd.lastPartitionKeys[serviceName]; ok {
		for _, key := range rd.lastPartitionKeys[serviceName] {
			delete(rd.services, key)
		}
		rd.lastPartitionKeys[serviceName] = rd.lastPartitionKeys[serviceName][:0]
	}
	rd.partitionKeyLock.Unlock()
	rd.servicesLock.Unlock()

	if _, ok := rd.lastPartitionKeys[serviceName]; !ok {
		rd.lastPartitionKeys[serviceName] = make([]int64, 0)
	}
	for key, serverList := range partitionServers {
		rd.servicesLock.Lock()
		rd.services[key] = serverList
		rd.partitionKeyLock.Lock()
		rd.lastPartitionKeys[serviceName] = append(rd.lastPartitionKeys[serviceName], key)
		rd.partitionKeyLock.Unlock()
		rd.servicesLock.Unlock()
	}
	for key, serverList := range shardServers {
		rd.servicesLock.Lock()
		rd.services[key] = serverList
		rd.servicesLock.Unlock()
	}
}

func (rd *RouterDbImpl) UpdateConfig(serviceName string, config ServiceConfig) {
	rd.configsLock.RLock()
	defer rd.configsLock.RUnlock()
	rd.configs[serviceName] = config
}

func (rd *RouterDbImpl) ServiceNotify(serviceName string, services ServerList) {
	rd.UpdateServers(serviceName, services)
}

func (rd *RouterDbImpl) ConfigNotify(serviceName string, serviceConfig ServiceConfig) {
	rd.UpdateConfig(serviceName, serviceConfig)
}

func (rd *RouterDbImpl) BatchSelectServers(serviceName, protocol string, routerIds []int64, shardType string, dc string) map[int64]ServerList {
	result := make(map[int64]ServerList)
	for _, routerId := range routerIds {
		serverList, ok := rd.SelectServers(serviceName, protocol, routerId, shardType, dc)
		if ok {
			result[routerId] = serverList
		}
	}
	return result
}

func (rd *RouterDbImpl) SelectServers(serviceName string, protocol string, routerId int64, shardType string, dc string) (ServerList, bool) {
	key := GetServiceKey(serviceName, protocol, routerId, shardType, dc)
	rd.servicesLock.RLock()
	defer rd.servicesLock.RUnlock()
	serverList, ok := rd.services[key]
	if ok {
		var results = make(ServerList, len(serverList))
		copy(results, serverList)
		return results, ok
	} else {
		return nil, false
	}
}
