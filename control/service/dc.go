package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type Dc struct {
	Id          uint32  `gorm:"primary_key"`
	ClusterId   uint32  `gorm:"type:int(32);not null"`
	ClusterInfo Cluster `gorm:"ForeignKey:ClusterId"`
	Name        string  `gorm:"type:varchar(255);not null;unique_index"`
	ShardNumber uint32  `gorm:"type:int(32);not null;"`
	Groups      []Group `gorm:"ForeignKey:DcId;AssociationForeignKey:Id"`
	Desc        string  `gorm:"type:varchar(255);"`
	CreatedAt   time.Time
}

type DcModel struct {
	ctx *context.Context
}

func NewDcModel(ctx *context.Context) *DcModel {
	instance := &DcModel{
		ctx: ctx,
	}
	return instance
}

func (dc *DcModel) Store(params params.DcParams) *common.Status {
	db := dc.ctx.Db()
	if status := dc.CheckDupName(params.Name); !status.Ok() {
		return status
	}
	if err := db.Create(&Dc{
		Name:        params.Name,
		ShardNumber: params.ShardNumber,
		Desc:        params.Desc,
		ClusterId:   params.ClusterId,
	}).Error; err != nil {
		return common.StatusWithError(err)
	}

	return GetNodeModel(dc.ctx).UpdateClusterInfo()
}

func (dc *DcModel) CheckDupName(name string) *common.Status {
	var count int
	db := dc.ctx.Db().Model(&Dc{})
	if err := db.Where(&Dc{Name: name}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if count != 0 {
		return common.StatusError(common.DcNameDup)
	}
	return common.StatusOk()
}

func (dc *DcModel) List(params params.DcListParams, isTotal bool) ([]Dc, uint32) {
	var (
		total uint32
		dcs   []Dc
	)
	db := dc.ctx.Db().Model(&Dc{})

	if isTotal {
		db.Count(&total)
	}

	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		db = db.Offset(offset).Limit(params.Limit)
	}
	db.Preload("ClusterInfo").Preload("Groups").Find(&dcs)
	return dcs, total
}

func (dc *DcModel) Update(params params.DcParams) *common.Status {
	db := dc.ctx.Db()
	if status := dc.CheckDupName(params.Name); status.Code() != common.DcNameDup {
		return common.StatusError(common.DcNotExist)
	}
	if err := db.Model(&Dc{Id: params.Id}).UpdateColumns(Dc{
		Name: params.Name,
		Desc: params.Desc,
	}).Error; err != nil {
		return common.StatusWithError(err)
	}

	return GetNodeModel(dc.ctx).UpdateClusterInfo()
}
