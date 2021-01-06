package service

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"net/http"
	"sort"
	"strconv"
	"sync"
)

type ShardMetrics struct {
	ShardId   uint32
	ReduceNum float64
}
type ReduceMetricsInfo struct {
	Address string
	Metrics []ShardMetrics
}

type ReduceModel struct {
	ctx                 *context.Context
	shardMetricsManager *ShardMetricsManager // 从线上拉取的metrics信息
}

var (
	metricsManager     *ReduceModel
	metricsManagerOnce sync.Once
)

func NewReduceModel(ctx *context.Context) *ReduceModel {
	metricsManagerOnce.Do(func() {
		instance := &ReduceModel{
			ctx:                 ctx,
			shardMetricsManager: GetShardMetricsManager(ctx),
		}
		metricsManager = instance
	})
	return metricsManager
}

func (reduce *ReduceModel) getTotalMetricsValue(data map[uint64](map[string]ShardInfo), reduceMode string) float64 {
	var value float64
	for _, hostMetrics := range data {
		for _, shardInfo := range hostMetrics {
			value += shardInfo.Metrics[reduceMode]
		}
	}
	return value
}

func (reduce *ReduceModel) sortByMode(nodeHash uint64, shardList map[string]ShardInfo, mode string) map[string]ShardInfo {
	shardMetricsValue := make([]ShardInfo, 0)
	retValue := make(map[string]ShardInfo)
	for _, shardMetrics := range shardList {
		shardMetricsValue = append(shardMetricsValue, shardMetrics)
	}
	sort.SliceStable(shardMetricsValue, func(i, j int) bool {
		if shardMetricsValue[i].Metrics[mode] < shardMetricsValue[j].Metrics[mode] {
			return true
		}
		return false
	})

	for _, metrics := range shardMetricsValue {
		shardIdHash := GetShardHash(nodeHash, metrics.ShardId)
		retValue[shardIdHash] = metrics
	}

	return retValue
}

func (reduce *ReduceModel) reduceMetricsHandle(groupId uint32, leftRate float64, reduceMode string) map[string](map[string]ShardInfo) {
	hostInfo, hostMetrics := reduce.getAllHostShardInfo(groupId)
	reduce.ctx.Log().Info(fmt.Sprint("reduce Mode is : ", reduceMode, "left rate is : ", leftRate))
	totalMetricsValue := reduce.getTotalMetricsValue(hostMetrics, reduceMode)
	reduce.ctx.Log().Info(fmt.Sprint("总体 value(不包含主节点的shard以及主shard) : ", totalMetricsValue))
	leftValuePerHost := totalMetricsValue * leftRate / float64(len(hostInfo))
	reduce.ctx.Log().Info(fmt.Sprint("目标单机 value : ", leftValuePerHost))
	result := make(map[string](map[string]ShardInfo))
	for hostInfoHash, hostValue := range hostInfo {
		var currentHostMetricsValue float64
		reduceShardList := make(map[string]ShardInfo)
		for _, shardInfo := range hostMetrics[hostInfoHash] {
			currentHostMetricsValue += shardInfo.Metrics[reduceMode]
		}
		// 由于key是shardIdHash，所以不必太关心key的顺序，当前是对value排序，然后重新生成一个map
		reduce.sortByMode(hostInfoHash, hostMetrics[hostInfoHash], reduceMode)
		ReduceValueLeft := currentHostMetricsValue - leftValuePerHost
		if ReduceValueLeft < 0 {
			reduce.ctx.Log().Info(fmt.Sprint("host currentHostMetricsValue : ", currentHostMetricsValue, " is too low"))
			continue
		}
		var hostReduceValue float64
		for shardHash, shardInfo := range hostMetrics[hostInfoHash] {
			flag := reduce.checkShardCanDisable(shardInfo, reduceShardList, shardInfo.Metrics[reduceMode], ReduceValueLeft)
			if flag == true {
				reduceShardList[shardHash] = shardInfo
				ReduceValueLeft -= shardInfo.Metrics[reduceMode]
				hostReduceValue += shardInfo.Metrics[reduceMode]
				if ReduceValueLeft <= 0 {
					break
				}
			}
		}
		address := hostValue.Host + ":" + strconv.Itoa(int(hostValue.Port))
		result[address] = reduceShardList
	}
	return result
}

func (reduce *ReduceModel) GetSpecShardNum(id uint32, reduceShard map[string]ShardInfo) uint32 {
	var num uint32 = 0
	for _, shardInfo := range reduceShard {
		if shardInfo.ShardId == id {
			num++
		}
	}
	return num
}

func (reduce *ReduceModel) checkShardCanDisable(info ShardInfo, reduceShard map[string]ShardInfo, currentValue float64, shouldDisableValue float64) bool {
	if info.Role == Leader {
		reduce.ctx.Log().Info(fmt.Sprint("主分片不能disable shards"))
		return false
	}
	if 0 < currentValue && currentValue < shouldDisableValue {
		shardNumInCache := reduce.shardMetricsManager.GetSpecShardNum(info.ShardId, Follower)
		shardNumInDisable := reduce.GetSpecShardNum(info.ShardId, reduceShard)
		if (shardNumInCache - shardNumInDisable) > 1 {
			return true
		}
	}
	return false
}
func (reduce *ReduceModel) disableShardMetricsHandle(result map[string](map[string]ShardInfo)) {
	for address, hostMetrics := range result {
		shardIdList := make([]uint32, 0)
		// 清空原有的数据
		hostMetricsJson, _ := json.Marshal(shardIdList)
		status := reduce.disableShardList(address, hostMetricsJson)
		if status.Code() != common.OK {
			reduce.ctx.Log().Error(fmt.Sprint("failed : ", status))
		}
		for _, info := range hostMetrics {
			shardIdList = append(shardIdList, info.ShardId)
		}
		hostMetricsJson, _ = json.Marshal(shardIdList)
		status = reduce.disableShardList(address, hostMetricsJson)
		if status.Code() != common.OK {
			reduce.ctx.Log().Error(fmt.Sprint("disable failed : ", status))
		}
	}
}

func (reduce *ReduceModel) disableShardList(host string, info []byte) *common.Status {
	url := "http://" + host + "/shard/unavailable"
	request, err := http.NewRequest("POST", url, bytes.NewBuffer(info))
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
		return common.StatusWithMessage(common.ShardDupFailed, response.Status)
	}
	return common.StatusOk()
}

func (reduce *ReduceModel) getAllHostShardInfo(groupId uint32) (map[uint64]NodeBasicInfo, map[uint64]map[string]ShardInfo) {
	var params params.NodeListParams
	params.GroupId = groupId
	node := GetNodeModel(reduce.ctx)
	nodeList, _ := node.list(params, false)
	nodeBasicInfo := make(map[uint64]NodeBasicInfo)
	nodeShardMetrics := make(map[uint64](map[string]ShardInfo))
	for _, nodeInfo := range nodeList {
		groupName := nodeInfo.GroupInfo.Name
		nodeHash := GetNodeHash(groupName, nodeInfo.NodeId)
		nodeInfoBasic, nodeMetricsInfo := reduce.shardMetricsManager.GetNodeShardInfo(groupName, nodeInfo.NodeId)
		shardMetricsList := make(map[string]ShardInfo)
		if *nodeInfoBasic.Master == true {
			reduce.ctx.Log().Info(fmt.Sprint("主节点不能disable shards, host : ", nodeInfoBasic.Host))
			continue
		}
		for shardHash, shardMetrics := range nodeMetricsInfo {
			if shardMetrics.Role == Leader {
				reduce.ctx.Log().Info(fmt.Sprint("主shard不能disable, shardId : ", shardMetrics.ShardId))
				continue
			}
			shardMetricsList[shardHash] = shardMetrics
		}
		nodeBasicInfo[nodeHash] = nodeInfoBasic
		nodeShardMetrics[nodeHash] = shardMetricsList
	}
	return nodeBasicInfo, nodeShardMetrics
}

func (reduce *ReduceModel) reduceAction(params params.GroupReduceMerticsParams) map[string](map[string]ShardInfo) {
	leftRate := float64(params.ReduceRate) / 100.0
	reduceMode := params.ReduceMode
	groupId := params.GroupId
	var result map[string](map[string]ShardInfo)
	if params.ReduceRate != 100 {
		result = reduce.reduceMetricsHandle(groupId, leftRate, reduceMode)
	}
	reduce.disableShardMetricsHandle(result)
	return result
}

func (reduce *ReduceModel) ReduceMetrics(params params.GroupReduceMerticsParams) []ReduceMetricsInfo {
	result := reduce.reduceAction(params)
	// 计算每个节点 --  节点所有的shardId 对应reduce的值，放入map中
	info := make([]ReduceMetricsInfo, 0)
	for address, hostMetrics := range result {
		value := make([]ShardMetrics, 0)
		for _, shardMetric := range hostMetrics {
			value = append(value, ShardMetrics{
				ShardId:   shardMetric.ShardId,
				ReduceNum: shardMetric.Metrics[params.ReduceMode],
			})
		}
		info = append(info, ReduceMetricsInfo{
			Address: address,
			Metrics: value,
		})
	}
	return info
}
