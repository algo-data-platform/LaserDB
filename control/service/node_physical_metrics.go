package service

import (
	"fmt"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type NodePhysicalMetrics struct {
	Id                 int32   `gorm:"primary_key"`
	ServiceAddress     string  `gorm:"type:varchar(255);null"`
	CpuUsageRate       float32 `gorm:"type:float;null"`
	CpuUsageRateUser   float32 `gorm:"type:float;null"`
	CpuUsageRateSystem float32 `gorm:"type:float;null"`
	MemorySizeTotal    int64   `gorm:"type:bigint(20);null"`
	MemorySizeUsage    int64   `gorm:"type:bigint(20);null"`
	MemoryUsageRate    float32 `gorm:"type:float;null"`
	DiskSizeTotal      int64   `gorm:"type:bigint(20);null"`
	DiskSizeUsage      int64   `gorm:"type:bigint(20);null"`
	DiskUsageRate      float32 `gorm:"type:float;null"`
	IoUsageRate        float32 `gorm:"type:float;null"`
	NetworkIn          int64   `gorm:"type:bigint(20);null"`
	NetworkOut         int64   `gorm:"type:bigint(20);null"`
	CollectTime        time.Time
	TimeType           int32 `gorm:"type:int(32);null"` // 1-平峰，2-高峰
}

type NodePhysicalMetricsModel struct {
	ctx *context.Context
}

func NewNodePhysicalMetricsModel(ctx *context.Context) *NodePhysicalMetricsModel {
	return &NodePhysicalMetricsModel{
		ctx: ctx,
	}
}

func (node *NodePhysicalMetricsModel) Insert(metricsInfo params.NodePhysicalMetricsInfo) *common.Status {
	addInfo := &NodePhysicalMetrics{
		ServiceAddress:     metricsInfo.ServiceAddress,
		CpuUsageRate:       metricsInfo.CpuUsageRate,
		CpuUsageRateUser:   metricsInfo.CpuUsageRateUser,
		CpuUsageRateSystem: metricsInfo.CpuUsageRateSystem,
		MemorySizeTotal:    metricsInfo.MemorySizeTotal,
		MemorySizeUsage:    metricsInfo.MemorySizeUsage,
		MemoryUsageRate:    metricsInfo.MemoryUsageRate,
		DiskSizeTotal:      metricsInfo.DiskSizeTotal,
		DiskSizeUsage:      metricsInfo.DiskSizeUsage,
		DiskUsageRate:      metricsInfo.DiskUsageRate,
		IoUsageRate:        metricsInfo.IoUsageRate,
		NetworkIn:          metricsInfo.NetworkIn,
		NetworkOut:         metricsInfo.NetworkOut,
		TimeType:           metricsInfo.TimeType,
	}
	addInfo.CollectTime = time.Unix(metricsInfo.CollectTime, 0)

	if err := node.ctx.Db().Create(addInfo).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (node *NodePhysicalMetricsModel) List(listParams params.NodePhysicalMetricsListParams) ([]NodePhysicalMetrics, uint32) {
	db := node.ctx.Db().Model(&NodePhysicalMetrics{}).Where(&NodePhysicalMetrics{TimeType: listParams.TimeType})

	if len(listParams.ServiceAddresses) > 0 {
		db = db.Where("service_address IN (?)", listParams.ServiceAddresses)
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

	var nodes []NodePhysicalMetrics
	db.Order("collect_time desc").Find(&nodes)

	return nodes, total
}

func (node *NodePhysicalMetricsModel) ListNodeList(listParams params.NodeAddressListParams) []params.MetricsNodeAddress {
	db := node.ctx.Db().Model(&NodePhysicalMetrics{})

	filterSql := "select distinct service_address from node_physical_metrics"
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
	filterSql = filterSql + " order by service_address"

	var distinctNodes []params.MetricsNodeAddress
	db.Raw(filterSql).Scan(&distinctNodes)

	return distinctNodes
}
