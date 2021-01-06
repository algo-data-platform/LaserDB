package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
)

const (
	TypeDbOptions    uint32 = 1
	TypeTableOptions uint32 = 2
	TypeCfOptions    uint32 = 3
)

const (
	DefaultConfigName string = "default"
)

type TableConfigItem struct {
	Id       uint32 `gorm:"primary_key" json:"Id" binding:"gte=0"`
	Name     string `gorm:"type:varchar(255);not null;unique_index:name_config_id" json:"Name" binding:"required"`
	Value    string `gorm:"type:varchar(256);not null" json:"Value" binding:"required"`
	Type     uint32 `gorm:"type:int(32);not null" json:"Type" binding:"required"` // 1-DbOptions, 2-TableOptions, 3-CfOptions
	ConfigId uint32 `gorm:"type:int(32);unique_index:name_config_id;default:0" json:"ConfigId" binding:"gte=0"`
}

type TableConfig struct {
	Id          uint32            `gorm:"primary_key"`
	Name        string            `gorm:"type:varchar(255);not null;unique"`
	Version     uint32            `gorm:"type:int(32);not null;"`
	IsDefault   *uint32           `gorm:"type:int(32);not null; default 0"` // 0-非默认配置，1-默认配置
	Desc        string            `gorm:"type:varchar(256);"`
	ConfigItems []TableConfigItem `gorm:"ForeignKey:ConfigId;AssociationForeignKey:Id"`
}

type TableConfigModel struct {
	ctx *context.Context
}

func NewTableConfigModel(ctx *context.Context) *TableConfigModel {
	instance := &TableConfigModel{
		ctx: ctx,
	}
	return instance
}

func (config *TableConfigModel) Store(params params.TableConfigParams) *common.Status {
	db := config.ctx.Db()
	if status := config.checkDupName(params.Name); !status.Ok() {
		return status
	}

	tableConfigItems := make([]TableConfigItem, 0)
	for _, item := range params.ConfigItems {
		tableConfigItem := TableConfigItem{
			Name:  item.Name,
			Value: item.Value,
			Type:  item.Type,
		}
		tableConfigItems = append(tableConfigItems, tableConfigItem)
	}

	configAdd := &TableConfig{
		Name:        params.Name,
		Version:     params.Version,
		IsDefault:   params.IsDefault,
		Desc:        params.Desc,
		ConfigItems: tableConfigItems,
	}

	if err := db.Create(configAdd).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (config *TableConfigModel) List(params params.TableConfigListParams, isTotal bool) ([]TableConfig, uint32) {
	if params.ExcludedNames == nil {
		params.ExcludedNames = []string{""}
	}
	db := config.ctx.Db().Model(&TableConfig{}).Where(&TableConfig{Name: params.Name}).Not("name", params.ExcludedNames)

	var total uint32
	var configs []TableConfig
	if isTotal {
		db.Count(&total)
	}
	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		db = db.Offset(offset).Limit(params.Limit)
	}
	db.Find(&configs)
	return configs, total
}

func (config *TableConfigModel) ListWithDetailItems(params params.TableConfigListParams, isTotal bool) ([]TableConfig, uint32) {
	if params.ExcludedNames == nil {
		params.ExcludedNames = []string{""}
	}
	db := config.ctx.Db().Model(&TableConfig{}).Where(&TableConfig{Name: params.Name}).Not("name", params.ExcludedNames)

	var total uint32
	var configs []TableConfig
	if isTotal {
		db.Count(&total)
	}
	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		db = db.Offset(offset).Limit(params.Limit)
	}
	db.Preload("ConfigItems").Find(&configs)
	return configs, total
}

func (config *TableConfigModel) Update(params params.TableConfigParams) *common.Status {
	db := config.ctx.Db()
	if status := config.checkDupName(params.Name); status.Code() != common.TableConfigNameDup {
		return common.StatusError(common.TableConfigNotExists)
	}

	tableConfigItems := make([]TableConfigItem, 0)
	for _, item := range params.ConfigItems {
		tableConfigItem := TableConfigItem{
			Name:  item.Name,
			Value: item.Value,
			Type:  item.Type,
		}
		tableConfigItems = append(tableConfigItems, tableConfigItem)
	}

	tx := db.Begin()
	if err := tx.Model(&TableConfig{Id: params.Id}).Association("ConfigItems").Clear().Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Model(&TableConfig{Id: params.Id}).Association("ConfigItems").Replace(tableConfigItems).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Unscoped().Where("config_id = 0 or config_id is NULL").Delete(&TableConfigItem{}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Model(&TableConfig{Id: params.Id}).Updates(&TableConfig{
		Name:    params.Name,
		Version: params.Version,
		Desc:    params.Desc,
	}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()

	return common.StatusOk()
}

func (config *TableConfigModel) Delete(params params.TableConfigDeleteParams) *common.Status {
	db := config.ctx.Db()
	tx := db.Begin()

	var usedCount uint32
	tx.Table("table_configs").Where("table_configs.id = ?", params.Id).
		Joins("Join tables on tables.config_id = table_configs.id").Count(&usedCount)
	if usedCount > 0 {
		tx.Rollback()
		return common.StatusError(common.TableConfigInUse)
	}
	if err := tx.Model(&TableConfig{Id: params.Id}).Association("ConfigItems").Clear().Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Unscoped().Where("config_id = 0 or config_id is NULL").Delete(&TableConfigItem{}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}

	if err := db.Delete(&TableConfig{Id: params.Id}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (config *TableConfigModel) checkDupName(configName string) *common.Status {
	var count int
	db := config.ctx.Db().Model(&TableConfig{})
	if err := db.Model(&TableConfig{}).Where(&TableConfig{Name: configName}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}

	if count != 0 {
		return common.StatusError(common.TableConfigNameDup)
	}
	return common.StatusOk()
}
