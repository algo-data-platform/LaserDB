package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type SystemIndex struct {
	Id                     int32   `gorm:"primary_key"`
	IoRatePerMb            float32 `gorm:"type:float;null"`
	NetAmplificationRatio  float32 `gorm:"type:float;null"`
	DiskAmplificationRatio float32 `gorm:"type:float;null"`
	CollectTime            time.Time
	TimeType               int32 `gorm:"type:int(32);null"` // 1-平峰，2-高峰
}

type SystemIndexModel struct {
	ctx *context.Context
}

func NewSystemIndexModel(ctx *context.Context) *SystemIndexModel {
	return &SystemIndexModel{
		ctx: ctx,
	}
}

func (manager *SystemIndexModel) Insert(metricsInfo params.SystemIndexInfo) *common.Status {
	addInfo := &SystemIndex{
		IoRatePerMb:            metricsInfo.IoRatePerMb,
		NetAmplificationRatio:  metricsInfo.NetAmplificationRatio,
		DiskAmplificationRatio: metricsInfo.DiskAmplificationRatio,
		TimeType:               metricsInfo.TimeType,
	}
	addInfo.CollectTime = time.Unix(metricsInfo.CollectTime, 0)

	if err := manager.ctx.Db().Create(addInfo).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (manager *SystemIndexModel) List(listParams params.SystemIndexListParams) ([]SystemIndex, uint32) {
	db := manager.ctx.Db().Model(&SystemIndex{}).Where(&SystemIndex{TimeType: listParams.TimeType})

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

	var systemIndexs []SystemIndex
	db.Find(&systemIndexs)

	return systemIndexs, total
}
