package service

import (
	"encoding/json"
	"reflect"
	"strconv"
	"strings"

	"laser-control/common"
	laserContext "laser-control/context"
	"laser-control/params"
)

type ConfigValidator struct {
	ctx *laserContext.Context
}

func NewConfigValidator(ctx *laserContext.Context) *ConfigValidator {
	instance := &ConfigValidator{
		ctx: ctx,
	}
	return instance
}

func (validator *ConfigValidator) CheckGroupReadyToBeMaster(param params.CheckGroupReadyToBeMasterParams) (*params.CheckGroupReadyToBeMasterResult, *common.Status) {
	clusterModel := NewClusterModel(validator.ctx)
	clusters, total := clusterModel.List(params.ClusterListParams{}, true)
	if total < 1 {
		return nil, common.StatusError(common.ClusterNotExist)
	}

	shardNumber := clusters[0].ShardTotal
	totalShards := make(map[uint32]bool)
	for shardId := uint32(0); shardId < shardNumber; shardId++ {
		totalShards[shardId] = true
	}

	result := &params.CheckGroupReadyToBeMasterResult{
		Ready:               false,
		MssingShards:        make([]uint32, 0),
		ReduplicativeShards: make([]uint32, 0),
		UnavailableShards:   make(map[uint32][]uint32, 0),
	}

	shardManager := GetShardModelNoSafe()
	shardList := shardManager.GetShards()
	for _, shardInfo := range shardList.Shards {
		if shardInfo.GroupName == param.GroupName {
			// check reduplication
			_, ok := totalShards[shardInfo.ShardId]
			if !ok {
				result.ReduplicativeShards = append(result.ReduplicativeShards, shardInfo.ShardId)
			} else {
				delete(totalShards, shardInfo.ShardId)
			}

			// check available state
			if shardInfo.ServiceState == Unavailable {
				result.UnavailableShards[shardInfo.NodeId] = append(result.UnavailableShards[shardInfo.NodeId],
					shardInfo.ShardId)
			}
		}
	}

	// check missing shards
	if len(totalShards) != 0 {
		for shardId := range totalShards {
			result.MssingShards = append(result.MssingShards, shardId)
		}
	}

	inconsistentNodes, status := validator.CheckGroupShardDifferenceBetweenLocalAndConsul(param.GroupName)
	if !status.Ok() {
		return nil, status
	}
	result.InconsistentNodes = inconsistentNodes

	if len(result.MssingShards) == 0 && len(result.ReduplicativeShards) == 0 &&
		len(result.UnavailableShards) == 0 && len(result.InconsistentNodes) == 0 {
		result.Ready = true
	}

	return result, common.StatusOk()
}

func (validator *ConfigValidator) StrictCheckGroupShardDifferenceBetweenLocalAndConsul(groupName string) []uint32 {
	inconsistentNodes := make([]uint32, 0)
	groupModel := NewGroupModel(validator.ctx)
	groups, total := groupModel.List(params.GroupListParams{GroupName: groupName, ShowDetails: true}, true)
	if total != 1 {
		return inconsistentNodes
	}

	metricsManager := GetShardMetricsManager(validator.ctx)
	group := groups[0]
	for _, node := range group.Nodes {
		localLeaderShardList := strings.Split(*node.LeaderShardList, ",")
		localLeaderShardMap := make(map[uint32]bool, 0)
		for _, shardIdStr := range localLeaderShardList {
			shardId, err := strconv.ParseUint(shardIdStr, 10, 32)
			if err != nil {
				continue
			}
			localLeaderShardMap[uint32(shardId)] = true
		}

		localFollowerShardList := strings.Split(*node.FollowerShardList, ",")
		localFollowerShardMap := make(map[uint32]bool, 0)
		for _, shardIdStr := range localFollowerShardList {
			shardId, err := strconv.ParseUint(shardIdStr, 10, 32)
			if err != nil {
				continue
			}
			localFollowerShardMap[uint32(shardId)] = true
		}

		inconsistent := false
		_, shardInfos := metricsManager.GetNodeShardInfo(groupName, node.NodeId)
		for _, shardInfo := range shardInfos {
			if shardInfo.Role == Leader {
				_, ok := localLeaderShardMap[shardInfo.ShardId]
				if !ok {
					inconsistent = true
					break
				}
				delete(localLeaderShardMap, shardInfo.ShardId)
			} else if shardInfo.Role == Follower {
				_, ok := localFollowerShardMap[shardInfo.ShardId]
				if !ok {
					inconsistent = true
					break
				}
				delete(localFollowerShardMap, shardInfo.ShardId)
			}
		}

		if len(localLeaderShardMap) != 0 || len(localFollowerShardMap) != 0 || inconsistent == true {
			inconsistentNodes = append(inconsistentNodes, node.NodeId)
		}
	}

	return inconsistentNodes
}

func (validator *ConfigValidator) CheckGroupShardDifferenceBetweenLocalAndConsul(groupName string) ([]uint32, *common.Status) {
	inconsistentNodes := make([]uint32, 0)
	groupModel := NewGroupModel(validator.ctx)
	groups, total := groupModel.List(params.GroupListParams{GroupName: groupName, ShowDetails: true}, true)
	if total != 1 {
		return inconsistentNodes, common.StatusError(common.GroupNotExist)
	}

	metricsManager := GetShardMetricsManager(validator.ctx)
	group := groups[0]
	for _, node := range group.Nodes {
		leaderShardList := extractUniqueShards(*node.LeaderShardList)
		followerShardList := extractUniqueShards(*node.FollowerShardList)

		// 目前节点不能同时拥有主分片和从分片
		if len(leaderShardList) != 0 && len(followerShardList) != 0 {
			inconsistentNodes = append(inconsistentNodes, node.NodeId)
			continue
		}

		var localShardList []string
		if len(leaderShardList) != 0 {
			localShardList = leaderShardList
		} else {
			localShardList = followerShardList
		}
		localShardMap := make(map[uint32]bool, 0)
		for _, shardIdStr := range localShardList {
			shardId, err := strconv.ParseUint(shardIdStr, 10, 32)
			if err != nil {
				continue
			}
			localShardMap[uint32(shardId)] = true
		}

		inconsistent := false
		_, shardInfos := metricsManager.GetNodeShardInfo(groupName, node.NodeId)
		for _, shardInfo := range shardInfos {
			_, ok := localShardMap[shardInfo.ShardId]
			if !ok {
				inconsistent = true
				break
			}
			delete(localShardMap, shardInfo.ShardId)
		}

		if len(localShardMap) != 0 || inconsistent == true {
			inconsistentNodes = append(inconsistentNodes, node.NodeId)
		}
	}

	return inconsistentNodes, common.StatusOk()
}

func (validator *ConfigValidator) CheckUnsynchronizedConfig() *params.CheckUnsynchronizedConfigReuslt {
	result := &params.CheckUnsynchronizedConfigReuslt{
		UnsynchronizedConfigTypes: make(map[uint32]bool),
	}
	consul := NewConsulModel(validator.ctx)
	unsynchronizedConfigs := make([]string, 0)
	for configKey, configGenerateFunc := range consul.consulKeyToGeneratorFunc {
		if consulValue, status := consul.consulGetLaserConfigsKV(configKey); status.Ok() {
			if genValue, status := configGenerateFunc(); status.Ok() {
				if equal, err := validator.jsonBytesEqual(consulValue, genValue); !equal && err == nil {
					unsynchronizedConfigs = append(unsynchronizedConfigs, configKey)
					if configType, ok := LaserConfigKeyToType[configKey]; ok {
						result.UnsynchronizedConfigTypes[uint32(configType)] = true
					}
				}
			}
		}
	}

	return result
}

func (validator *ConfigValidator) jsonBytesEqual(a, b []byte) (bool, error) {
	var j, j2 interface{}
	if err := json.Unmarshal(a, &j); err != nil {
		return false, err
	}
	if err := json.Unmarshal(b, &j2); err != nil {
		return false, err
	}
	return reflect.DeepEqual(j2, j), nil
}
