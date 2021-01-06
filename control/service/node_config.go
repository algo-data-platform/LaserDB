package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type NodeRateLimitEntry struct {
	Id              uint32  `gorm:"primary_key"`
	BeginHour       *uint32 `gorm:"type:int(32);not null"`
	EndHour         uint32  `gorm:"type:int(32);not null"`
	RateBytesPerSec int64   `gorm:"type:bigint;not null"`
	NodeConfigId    uint32  `gorm:"type:int(32);not null"`
}

type NodeConfig struct {
	Id                  uint32               `gorm:"primary_key"`
	Name                string               `gorm:"type:varchar(255);not null"`
	Desc                string               `json:"Desc" binding:"-"`
	BlockCacheSizeGb    uint32               `sql:"type:int(32);not null"`
	WriteBufferSizeGb   uint32               `sql:"type:int(32);not null"`
	NumShardBits        int32                `sql:"type:int(32);not null"`
	HighPriPoolRatio    *float64             `sql:"type:double;not null;default 0"`
	StrictCapacityLimit uint32               `sql:"type:int(32);not null"`
	RateLimitStrategy   []NodeRateLimitEntry `gorm:"ForeignKey:NodeConfigId;AssociationForeignKey:Id"`
	CreatedAt           time.Time
}

type NodeConfigModel struct {
	ctx *context.Context
}

func NewNodeConfigModel(ctx *context.Context) *NodeConfigModel {
	instance := &NodeConfigModel{
		ctx: ctx,
	}
	return instance
}

func (node_config *NodeConfigModel) Store(params params.NodeConfigParams) *common.Status {
	db := node_config.ctx.Db()
	if status := node_config.checkDupName(params.Name); !status.Ok() {
		return status
	}

	rateLimitStrategy := make([]NodeRateLimitEntry, 0)
	for _, entry := range params.RateLimitStrategy {
		nodeRateLimitEntry := NodeRateLimitEntry{
			BeginHour:       entry.BeginHour,
			EndHour:         entry.EndHour,
			RateBytesPerSec: entry.RateBytesPerSec,
		}
		rateLimitStrategy = append(rateLimitStrategy, nodeRateLimitEntry)
	}

	configAdd := &NodeConfig{
		Name:                params.Name,
		Desc:                params.Desc,
		BlockCacheSizeGb:    params.BlockCacheSizeGb,
		WriteBufferSizeGb:   params.WriteBufferSizeGb,
		NumShardBits:        params.NumShardBits,
		HighPriPoolRatio:    params.HighPriPoolRatio,
		StrictCapacityLimit: params.StrictCapacityLimit,
		RateLimitStrategy:   rateLimitStrategy,
		CreatedAt:           time.Now(),
	}

	if err := db.Create(configAdd).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (node_config *NodeConfigModel) List(para params.NodeConfigListParams, isTotal bool) ([]NodeConfig, uint32) {
	db := node_config.ctx.Db().Model(&NodeConfig{})

	var total uint32
	var node_configs []NodeConfig
	if isTotal {
		db.Where(&NodeConfig{Name: para.Name}).Count(&total)
	}
	if para.Page > 0 && para.Limit > 0 {
		offset := (para.Page - 1) * para.Limit
		db = db.Offset(offset).Limit(para.Limit)
	}
	db.Preload("RateLimitStrategy").Where(&NodeConfig{Name: para.Name}).Find(&node_configs)
	return node_configs, total
}

func (node_config *NodeConfigModel) Update(params params.NodeConfigParams) *common.Status {
	db := node_config.ctx.Db()
	if status := node_config.checkDupName(params.Name); status.Code() != common.TableNameDup {
		return common.StatusError(common.TableNameDup)
	}

	rateLimitStrategy := make([]NodeRateLimitEntry, 0)
	for _, entry := range params.RateLimitStrategy {
		nodeRateLimitEntry := NodeRateLimitEntry{
			BeginHour:       entry.BeginHour,
			EndHour:         entry.EndHour,
			RateBytesPerSec: entry.RateBytesPerSec,
		}
		rateLimitStrategy = append(rateLimitStrategy, nodeRateLimitEntry)
	}
	tx := db.Begin()
	if err := tx.Model(&NodeConfig{Id: params.Id}).Association("RateLimitStrategy").Clear().Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Model(&NodeConfig{Id: params.Id}).Association("RateLimitStrategy").Replace(rateLimitStrategy).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Unscoped().Where("node_config_id = 0").Delete(&NodeRateLimitEntry{}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Model(&NodeConfig{Id: params.Id}).UpdateColumns(NodeConfig{
		BlockCacheSizeGb:    params.BlockCacheSizeGb,
		WriteBufferSizeGb:   params.WriteBufferSizeGb,
		NumShardBits:        params.NumShardBits,
		HighPriPoolRatio:    params.HighPriPoolRatio,
		StrictCapacityLimit: params.StrictCapacityLimit,
	}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()

	return common.StatusOk()
}

func (node_config *NodeConfigModel) Delete(params params.NodeConfigDeleteParams) *common.Status {
	dbModel := node_config.ctx.Db()
	tx := dbModel.Begin()

	var usedCount uint32
	tx.Table("node_configs").Where("node_configs.id = ?", params.Id).
		Joins("Join nodes on nodes.config_id = node_configs.id").Count(&usedCount)
	if usedCount > 0 {
		tx.Rollback()
		return common.StatusError(common.NodeConfigInUse)
	}
	if err := tx.Model(&NodeConfig{Id: params.Id}).Association("RateLimitStrategy").Clear().Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Unscoped().Where("node_config_id = 0").Delete(&NodeRateLimitEntry{}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := node_config.ctx.Db().Delete(&NodeConfig{Id: params.Id}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (node_config *NodeConfigModel) checkDupName(name string) *common.Status {
	var count int
	db := node_config.ctx.Db().Model(&NodeConfig{})
	if err := db.Model(&NodeConfig{}).Where(&NodeConfig{Name: name}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if count != 0 {
		return common.StatusError(common.TableNameDup)
	}
	return common.StatusOk()
}
