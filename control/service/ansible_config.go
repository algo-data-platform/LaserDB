package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type AnsibleConfig struct {
	Id        uint32 `gorm:"primary_key"`
	Name      string `gorm:"type:varchar(255);not null"`
	Vars      string `gorm:"type:text;not null"`
	Roles     string `gorm:"type:text;not null"`
	Desc      string `gorm:"type:varchar(256);"`
	CreatedAt time.Time
}

type AnsibleConfigModel struct {
	ctx *context.Context
}

func NewAnsibleConfigModel(ctx *context.Context) *AnsibleConfigModel {
	instance := &AnsibleConfigModel{
		ctx: ctx,
	}
	return instance
}

func (db *AnsibleConfigModel) Store(params params.AnsibleConfigParams) *common.Status {
	dbModel := db.ctx.Db()

	if status := db.CheckDupName(params.Name); !status.Ok() {
		return status
	}

	ansibleConfigAdd := &AnsibleConfig{
		Name:      params.Name,
		Vars:      params.Vars,
		Roles:     params.Roles,
		Desc:      params.Desc,
		CreatedAt: time.Now(),
	}

	if err := dbModel.Create(ansibleConfigAdd).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (db *AnsibleConfigModel) List(params params.AnsibleConfigListParams, isTotal bool) ([]AnsibleConfig, uint32) {
	dbModel := db.ctx.Db().Model(&AnsibleConfig{}).Where(&AnsibleConfig{Id: params.Id})

	var total uint32
	var configs []AnsibleConfig
	if isTotal {
		dbModel.Count(&total)
	}
	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		dbModel = dbModel.Offset(offset).Limit(params.Limit)
	}
	dbModel.Find(&configs)
	return configs, total
}

func (db *AnsibleConfigModel) Update(params params.AnsibleConfigParams) *common.Status {
	dbModel := db.ctx.Db()

	// 判断配置Id是否存在
	count := 0
	if err := dbModel.Model(&AnsibleConfig{Id: params.Id}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if (count == 0) {
		return common.StatusError(common.AnsibleConfigNotExists)
	}

	// 配置名称不可重复：需要检查更新后的名称是否与其他配置重名
	var dupItem AnsibleConfig
	if result := dbModel.Model(&AnsibleConfig{}).Where(&AnsibleConfig{Name: params.Name}).First(&dupItem); result.Error != nil {
		// 当没有找到名称相同的配置时，不返回异常
		if (!result.RecordNotFound()) {
			return common.StatusWithError(result.Error)
		}
	} else {
		// 找到了配置名相同的配置，且不是当前配置时，返回配置已存在
		if (dupItem.Id != params.Id) {
			return common.StatusError(common.AnsibleConfigDup)
		}
	}

	if err := dbModel.Model(&AnsibleConfig{Id: params.Id}).UpdateColumns(AnsibleConfig{
		Name:  params.Name,
		Vars:  params.Vars,
		Roles: params.Roles,
		Desc:  params.Desc,
	}).Error; err != nil {
		return common.StatusWithError(err)
	}

	return common.StatusOk()
}

func (db *AnsibleConfigModel) Delete(params params.AnsibleConfigDeleteParams) *common.Status {
	dbModel := db.ctx.Db()
	tx := dbModel.Begin()

	var usedCount uint32
	tx.Table("ansible_configs").Where("ansible_configs.id = ?", params.Id).
		Joins("Join nodes on nodes.ansible_config_id = ansible_configs.id").Count(&usedCount)
	if usedCount > 0 {
		tx.Rollback()
		return common.StatusError(common.AnsibleConfigInUse)
	}

	if err := tx.Delete(&AnsibleConfig{Id: params.Id}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()

	return common.StatusOk()
}

func (db *AnsibleConfigModel) CheckDupName(name string) *common.Status {
	var count int
	dbModel := db.ctx.Db().Model(&AnsibleConfig{})
	if err := dbModel.Model(&AnsibleConfig{}).Where(&AnsibleConfig{Name: name}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}

	if count != 0 {
		return common.StatusError(common.AnsibleConfigDup)
	}

	return common.StatusOk()
}
