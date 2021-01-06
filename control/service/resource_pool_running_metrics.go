package service

import (
	"fmt"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type ResourcePoolRunningMetrics struct {
	Id                       int32   `gorm:"primary_key"`
	PoolName                 string  `gorm:"type:varchar(255);null"`
	Qps                      int32   `gorm:"type:int(32);null"`
	Kps                      int32   `gorm:"type:int(32);null"`
	KpsWrite                 int32   `gorm:"type:int(32);null"`
	KpsRead                  int32   `gorm:"type:int(32);null"`
	Bps                      int64   `gorm:"type:bigint(20);null"`
	BpsWrite                 int64   `gorm:"type:bigint(20);null"`
	BpsRead                  int64   `gorm:"type:bigint(20);null"`
	DataSize                 int64   `gorm:"type:bigint(20);null"`
	P99                      float32 `gorm:"type:float;null"`
	P999                     float32 `gorm:"type:float;null"`
	JitterDuration           int32   `gorm:"type:int(32);null"`
	CapacityIdleRate         float32 `gorm:"type:float;null"`
	CapacityIdleRateAbsolute float32 `gorm:"type:float;null"`
	CollectTime              time.Time
	TimeType                 int32 `gorm:"type:int(32);null"` // 1-平峰，2-高峰
}

type ResourcePoolRunningMetricsModel struct {
	ctx *context.Context
}

func NewResourcePoolRunningMetricsModel(ctx *context.Context) *ResourcePoolRunningMetricsModel {
	return &ResourcePoolRunningMetricsModel{
		ctx: ctx,
	}
}

func (pool *ResourcePoolRunningMetricsModel) Insert(metricsInfo params.ResourcePoolRunningMetricsInfo) *common.Status {
	addInfo := &ResourcePoolRunningMetrics{
		PoolName:                 metricsInfo.PoolName,
		Qps:                      metricsInfo.Qps,
		Kps:                      metricsInfo.Kps,
		KpsWrite:                 metricsInfo.KpsWrite,
		KpsRead:                  metricsInfo.KpsRead,
		Bps:                      metricsInfo.Bps,
		BpsWrite:                 metricsInfo.BpsWrite,
		BpsRead:                  metricsInfo.BpsRead,
		DataSize:                 metricsInfo.DataSize,
		P99:                      metricsInfo.P99,
		P999:                     metricsInfo.P999,
		JitterDuration:           metricsInfo.JitterDuration,
		CapacityIdleRate:         metricsInfo.CapacityIdleRate,
		CapacityIdleRateAbsolute: metricsInfo.CapacityIdleRateAbsolute,
		TimeType:                 metricsInfo.TimeType,
	}
	addInfo.CollectTime = time.Unix(metricsInfo.CollectTime, 0)

	if err := pool.ctx.Db().Create(addInfo).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (pool *ResourcePoolRunningMetricsModel) List(listParams params.ResourcePoolRunningMetricsListParams) ([]ResourcePoolRunningMetrics, uint32) {
	db := pool.ctx.Db().Model(&ResourcePoolRunningMetrics{}).Where(&ResourcePoolRunningMetrics{TimeType: listParams.TimeType})

	if len(listParams.PoolNames) > 0 {
		db = db.Where("pool_name IN (?)", listParams.PoolNames)
	}
	if listParams.StartTime > 0 {
		startTimeUnix := time.Unix(listParams.StartTime, 0)
		db = db.Where("collect_time >= ?", startTimeUnix)
	}

	if listParams.EndTime > 0 {
		endTimeUnix := time.Unix(listParams.EndTime, 0)
		db = db.Where("collect_time <= ?", endTimeUnix)
	}

	var total uint32
	db.Count(&total)
	if listParams.Page > 0 && listParams.Limit > 0 {
		offset := (listParams.Page - 1) * listParams.Limit
		db = db.Offset(offset).Limit(listParams.Limit)
	}

	var resourcePoolRunningMetrics []ResourcePoolRunningMetrics
	db.Order("collect_time desc").Find(&resourcePoolRunningMetrics)

	return resourcePoolRunningMetrics, total
}

func (pool *ResourcePoolRunningMetricsModel) ListRunningPoolList(listParams params.MetricPoolListParams) []params.MetricsPoolName {
	db := pool.ctx.Db().Model(&ResourcePoolRunningMetrics{})

	filterSql := "select distinct pool_name from resource_pool_running_metrics"
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
	filterSql = filterSql + " order by pool_name"

	var distinctPools []params.MetricsPoolName
	db.Raw(filterSql).Scan(&distinctPools)

	return distinctPools
}
