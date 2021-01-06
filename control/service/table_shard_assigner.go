package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"sort"
)

type Pair struct {
	key   int64
	value uint64
}

type PairList []Pair

func (p PairList) Swap(i, j int)      { p[i], p[j] = p[j], p[i] }
func (p PairList) Len() int           { return len(p) }
func (p PairList) Less(i, j int) bool { return p[i].value < p[j].value }

type AssignedShardList struct {
	MetricSum uint64
	ShardList []int64
}
type AssignedShardLists []AssignedShardList

func (p AssignedShardLists) Swap(i, j int)      { p[i], p[j] = p[j], p[i] }
func (p AssignedShardLists) Len() int           { return len(p) }
func (p AssignedShardLists) Less(i, j int) bool { return p[i].MetricSum < p[j].MetricSum }

type AssignedResultEntry struct {
	SizeSum   uint64
	KpsSum    uint64
	ShardList []int64
}

type AssignedTableShardResult struct {
	TotalShardListInfo     AssignedResultEntry
	AssignedShardListInfos []AssignedResultEntry
}

type TableShardAssignerModel struct {
	ctx *context.Context
}

func NewTableShardAssignerModel(ctx *context.Context) *TableShardAssignerModel {
	instance := &TableShardAssignerModel{
		ctx: ctx,
	}
	return instance
}

func (model *TableShardAssignerModel) AssignTableShardList(param params.AssignShardListParams) (
	AssignedTableShardResult, *common.Status) {
	var assignedTableShardResult AssignedTableShardResult
	shardMetricsManager := GetShardMetricsManager(model.ctx)
	tableShardList := shardMetricsManager.GetTableShardList(param.DatabaseName, param.TableName)
	if param.AssignedListNum > uint32(len(tableShardList)) {
		return assignedTableShardResult, common.StatusError(common.AssignTableShardListNumTooLarge)
	}
	sort.Slice(tableShardList, func(i, j int) bool { return tableShardList[i] < tableShardList[j] })
	switch param.Type {
	case params.AssignByShardSize:
		assignedTableShardResult = model.assignShardListBySize(param)
	case params.AssignByShardKps:
		assignedTableShardResult = model.assignShardListByKps(param)
	case params.AssignByShardNum:
		assignedTableShardResult = model.assignShardListByNum(param)
	}

	return assignedTableShardResult, common.StatusOk()
}

func (model *TableShardAssignerModel) assignShardListByNum(param params.AssignShardListParams) AssignedTableShardResult {
	var assignedTableShardResult AssignedTableShardResult
	listNum := param.AssignedListNum
	resultEntries := make([]AssignedResultEntry, listNum)

	shardMetricsManager := GetShardMetricsManager(model.ctx)
	totalShardList := shardMetricsManager.GetTableShardList(param.DatabaseName, param.TableName)
	sort.Slice(totalShardList, func(i, j int) bool { return totalShardList[i] < totalShardList[j] })
	tableShardSize, tableShardKps := shardMetricsManager.GetTableShardSizeAndKps(param.DatabaseName, param.TableName)
	count := uint32(0)
	for _, shardId := range totalShardList {
		index := count % listNum
		resultEntries[index].SizeSum += tableShardSize[shardId]
		resultEntries[index].KpsSum += tableShardKps[shardId]
		resultEntries[index].ShardList = append(resultEntries[index].ShardList, shardId)
		assignedTableShardResult.TotalShardListInfo.SizeSum += tableShardSize[shardId]
		assignedTableShardResult.TotalShardListInfo.KpsSum += tableShardKps[shardId]
		count++
	}
	assignedTableShardResult.TotalShardListInfo.ShardList = totalShardList
	assignedTableShardResult.AssignedShardListInfos = resultEntries

	return assignedTableShardResult
}

func (model *TableShardAssignerModel) assignShardListBySize(param params.AssignShardListParams) AssignedTableShardResult {
	var assignedTableShardResult AssignedTableShardResult
	shardMetricsManager := GetShardMetricsManager(model.ctx)
	tableShardSize, tableShardKps := shardMetricsManager.GetTableShardSizeAndKps(param.DatabaseName, param.TableName)
	assignedShardLists := model.assignShardListByMetric(param.AssignedListNum, tableShardSize)

	totalShardList := make([]int64, 0)
	assignedShardListInfos := make([]AssignedResultEntry, 0)
	for _, shardList := range assignedShardLists {
		assignedResultEntry := AssignedResultEntry{
			SizeSum:   shardList.MetricSum,
			KpsSum:    0,
			ShardList: shardList.ShardList,
		}
		for _, shardId := range shardList.ShardList {
			assignedResultEntry.KpsSum += tableShardKps[shardId]
			totalShardList = append(totalShardList, shardId)
		}
		assignedTableShardResult.TotalShardListInfo.SizeSum += assignedResultEntry.SizeSum
		assignedTableShardResult.TotalShardListInfo.KpsSum += assignedResultEntry.KpsSum
		assignedShardListInfos = append(assignedShardListInfos, assignedResultEntry)
	}
	sort.Slice(totalShardList, func(i, j int) bool { return totalShardList[i] < totalShardList[j] })
	assignedTableShardResult.TotalShardListInfo.ShardList = totalShardList
	assignedTableShardResult.AssignedShardListInfos = assignedShardListInfos
	return assignedTableShardResult
}

func (model *TableShardAssignerModel) assignShardListByKps(param params.AssignShardListParams) AssignedTableShardResult {
	var assignedTableShardResult AssignedTableShardResult
	shardMetricsManager := GetShardMetricsManager(model.ctx)
	tableShardSize, tableShardKps := shardMetricsManager.GetTableShardSizeAndKps(param.DatabaseName, param.TableName)
	assignedShardLists := model.assignShardListByMetric(param.AssignedListNum, tableShardKps)

	totalShardList := make([]int64, 0)
	assignedShardListInfos := make([]AssignedResultEntry, 0)
	for _, shardList := range assignedShardLists {
		assignedResultEntry := AssignedResultEntry{
			SizeSum:   0,
			KpsSum:    shardList.MetricSum,
			ShardList: shardList.ShardList,
		}
		for _, shardId := range shardList.ShardList {
			assignedResultEntry.SizeSum += tableShardSize[shardId]
			totalShardList = append(totalShardList, shardId)
		}
		assignedTableShardResult.TotalShardListInfo.SizeSum += assignedResultEntry.SizeSum
		assignedTableShardResult.TotalShardListInfo.KpsSum += assignedResultEntry.KpsSum
		assignedShardListInfos = append(assignedShardListInfos, assignedResultEntry)
	}
	sort.Slice(totalShardList, func(i, j int) bool { return totalShardList[i] < totalShardList[j] })
	assignedTableShardResult.TotalShardListInfo.ShardList = totalShardList
	assignedTableShardResult.AssignedShardListInfos = assignedShardListInfos
	return assignedTableShardResult
}

func (model *TableShardAssignerModel) assignShardListByMetric(listNum uint32, shardMetric map[int64]uint64) AssignedShardLists {
	shardLists := make(AssignedShardLists, listNum)
	pairList := make(PairList, 0)
	for k, v := range shardMetric {
		pair := Pair{
			key:   k,
			value: v,
		}
		pairList = append(pairList, pair)
	}
	sort.Sort(sort.Reverse(pairList))

	for i := 0; i < len(pairList); {
		sort.Sort(shardLists)
		shardLists[0].MetricSum += pairList[i].value
		shardLists[0].ShardList = append(shardLists[0].ShardList, pairList[i].key)
		pairList = append(pairList[:i], pairList[i+1:]...)
	}

	for _, assignedShardList := range shardLists {
		shardList := assignedShardList.ShardList
		sort.Slice(shardList, func(i, j int) bool { return shardList[i] < shardList[j] })
	}

	return shardLists
}
