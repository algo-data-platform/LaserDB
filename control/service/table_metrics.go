package service

import (
	"fmt"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type TableMetrics struct {
	Id              int32  `gorm:"primary_key"`
	DatabaseName    string `gorm:"type:varchar(255);null"`
	TableName       string `gorm:"type:varchar(255);null"`
	DataSize        int64  `gorm:"type:bigint(20);null"`
	Kps             int32  `gorm:"type:int(32);null"`
	KpsWrite        int32  `gorm:"type:int(32);null"`
	KpsRead         int32  `gorm:"type:int(32);null"`
	Bps             int64  `gorm:"type:bigint(20);null"`
	BpsWrite        int64  `gorm:"type:bigint(20);null"`
	BpsRead         int64  `gorm:"type:bigint(20);null"`
	PartitionNumber int32  `gorm:"type:int(32);null"`
	CollectTime     time.Time
	TimeType        int32 `gorm:"type:int(32);null"` // 1-平峰，2-高峰
}

type TableMetricsModel struct {
	ctx *context.Context
}

func NewTableMetricsModel(ctx *context.Context) *TableMetricsModel {
	return &TableMetricsModel{
		ctx: ctx,
	}
}

func (table *TableMetricsModel) Insert(metricsInfo params.TableMetricsInfo) *common.Status {
	addInfo := &TableMetrics{
		DatabaseName:    metricsInfo.DatabaseName,
		TableName:       metricsInfo.TableName,
		DataSize:        metricsInfo.DataSize,
		Kps:             metricsInfo.Kps,
		KpsWrite:        metricsInfo.KpsWrite,
		KpsRead:         metricsInfo.KpsRead,
		Bps:             metricsInfo.Bps,
		BpsWrite:        metricsInfo.BpsWrite,
		BpsRead:         metricsInfo.BpsRead,
		PartitionNumber: metricsInfo.PartitionNumber,
		TimeType:        metricsInfo.TimeType,
	}
	addInfo.CollectTime = time.Unix(metricsInfo.CollectTime, 0)

	if err := table.ctx.Db().Create(addInfo).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (table *TableMetricsModel) List(listParams params.TableMetricsListParams) ([]TableMetrics, uint32) {
	db := table.ctx.Db().Model(&TableMetrics{})

	filterSql := "select * from table_metrics"
	filterPrefix := " where"
	if listParams.TimeType > 0 {
		tmp := fmt.Sprintf("%s time_type = %d", filterPrefix, listParams.TimeType)
		filterSql = filterSql + tmp
		filterPrefix = " and"
	}
	if listParams.StartTime > 0 {
		startTimeUnix := time.Unix(listParams.StartTime, 0)
		tmp := fmt.Sprintf("%s collect_time >= '%s'", filterPrefix, startTimeUnix)
		filterSql = filterSql + tmp
		filterPrefix = " and"
	}
	if listParams.EndTime > 0 {
		endTimeUnix := time.Unix(listParams.EndTime, 0)
		tmp := fmt.Sprintf("%s collect_time <= '%s'", filterPrefix, endTimeUnix)
		filterSql = filterSql + tmp
		filterPrefix = " and"
	}

	tableFilterSql := ""
	for _, table := range listParams.Tables {
		tmp := fmt.Sprintf("(database_name = '%s' and table_name = '%s') or ", table.DatabaseName, table.TableName)
		tableFilterSql = tableFilterSql + tmp
	}
	if len(tableFilterSql) > 0 {
		tableFilterSql = tableFilterSql[0:(len(tableFilterSql) - len(" or "))]
		filterSql = filterSql + filterPrefix + fmt.Sprintf(" (%s)", tableFilterSql)
		filterPrefix = " or"
	}

	filterSql = filterSql + " order by collect_time desc"

	db = db.Raw(filterSql)

	var total uint32
	db.Count(&total)
	if listParams.Page > 0 && listParams.Limit > 0 {
		offset := (listParams.Page - 1) * listParams.Limit
		db = db.Offset(offset).Limit(listParams.Limit)
	}

	var tableMetrics []TableMetrics
	db.Scan(&tableMetrics)

	return tableMetrics, total
}

func (table *TableMetricsModel) ListTableList(listParams params.MetricTableListParams) []params.MetricTable {
	db := table.ctx.Db().Model(&TableMetrics{})

	filterSql := "select distinct database_name, table_name from table_metrics"
	filterPrefix := " where"
	if listParams.TimeType > 0 {
		tmp := fmt.Sprintf("%s time_type = %d", filterPrefix, listParams.TimeType)
		filterSql = filterSql + tmp
		filterPrefix = " and"
	}
	if listParams.StartTime > 0 {
		startTimeUnix := time.Unix(listParams.StartTime, 0)
		tmp := fmt.Sprintf("%s collect_time >= '%s'", filterPrefix, startTimeUnix)
		filterSql = filterSql + tmp
		filterPrefix = " and"
	}
	if listParams.EndTime > 0 {
		endTimeUnix := time.Unix(listParams.EndTime, 0)
		tmp := fmt.Sprintf("%s collect_time <= '%s'", filterPrefix, endTimeUnix)
		filterSql = filterSql + tmp
		filterPrefix = " and"
	}

	var distinctTables []params.MetricTable
	db.Raw(filterSql).Scan(&distinctTables)

	return distinctTables
}
