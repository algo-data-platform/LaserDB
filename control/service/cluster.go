package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"

	"time"
)

type Cluster struct {
	Id         uint32  `gorm:"primary_key"`
	Name       string  `gorm:"type:varchar(255);not null;unique_index"`
	Alias      string  `gorm:"type:varchar(255);not null"`
	ShardTotal uint32  `gorm:"type:int(32);not null"`
	Desc       string  `gorm:"type:varchar(256);"`
	Groups     []Group `gorm:"ForeignKey:ClusterId;AssociationForeignKey:Id"`
	Dcs        []Dc    `gorm:"ForeignKey:ClusterId;AssociationForeignKey:Id"`
	CreatedAt  time.Time
}

type ClusterModel struct {
	ctx *context.Context
}

func NewClusterModel(ctx *context.Context) *ClusterModel {
	instance := &ClusterModel{
		ctx: ctx,
	}
	return instance
}

func (cluster *ClusterModel) Store(params params.ClusterParams) *common.Status {
	db := cluster.ctx.Db()
	if status := cluster.checkDupName(params.Name); !status.Ok() {
		return status
	}

	clusterInfo := &Cluster{
		Name:       params.Name,
		Alias:      params.Alias,
		ShardTotal: params.ShardTotal,
		Desc:       params.Desc,
	}

	if err := db.Create(clusterInfo).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (cluster *ClusterModel) List(params params.ClusterListParams, isTotal bool) ([]Cluster, uint32) {
	db := cluster.ctx.Db().Model(&Cluster{})
	var total uint32
	var clusters []Cluster
	if isTotal {
		db.Count(&total)
	}

	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		db = db.Offset(offset).Limit(params.Limit)
	}
	db.Preload("Groups").Preload("Dcs").Find(&clusters)
	return clusters, total
}

func (cluster *ClusterModel) Update(params params.ClusterParams) *common.Status {
	db := cluster.ctx.Db()
	if status := cluster.checkDupName(params.Name); status.Code() != common.ClusterNameDup {
		return common.StatusError(common.ClusterNotExist)
	}

	if err := db.Model(&Cluster{Id: params.Id}).UpdateColumns(Cluster{
		Alias: params.Alias,
		Desc:  params.Desc,
	}).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (cluster *ClusterModel) checkDupName(clusterName string) *common.Status {
	var count int
	db := cluster.ctx.Db().Model(&Cluster{})
	if err := db.Where(&Cluster{Name: clusterName}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if count != 0 {
		return common.StatusError(common.ClusterNameDup)
	}
	return common.StatusOk()
}
