package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type ProxyTableConfig struct {
	Id           uint32        `gorm:"primary_key"`
	DatabaseName string 	   `gorm:"type:varchar(255);not null"`
	TableName    string        `gorm:"type:varchar(255);not null"`
	ReadTimeout  uint32        `sql:"type:int(32);not null"`
	WriteTimeout uint32        `sql:"type:int(32);not null"`
	AllowedFlow  uint32        `sql:"type:int(32);not null"`
	CreatedAt    time.Time
}

var defaultDbName string = "global"
var defaultTbName string = "global"

type ProxyTableConfigModel struct {
	ctx *context.Context
}

func NewProxyTableConfigModel(ctx *context.Context) *ProxyTableConfigModel {
	instance := &ProxyTableConfigModel{
		ctx: ctx,
	}
	return instance
}

func (proxy_config *ProxyTableConfigModel) Store(params params.ProxyTableConfigParams) *common.Status {
	db := proxy_config.ctx.Db()

	if status := proxy_config.checkDupName(params.DatabaseName, params.TableName); !status.Ok() {
		if params.DatabaseName != defaultDbName {
			return status
		}
	}

	tableAdd := &ProxyTableConfig{
		TableName:    params.TableName,
		DatabaseName: params.DatabaseName,
		ReadTimeout:  params.ReadTimeout,
		WriteTimeout: params.WriteTimeout,
		AllowedFlow:  params.AllowedFlow,
		CreatedAt:    time.Now(),
	}
	if err := db.Create(tableAdd).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (proxy_config *ProxyTableConfigModel) List(para params.ProxyTableConfigListParams, isTotal bool) ([]ProxyTableConfig, uint32) {
	db := proxy_config.ctx.Db().Model(&ProxyTableConfig{})

	var total uint32
	var proxy_configs []ProxyTableConfig
	db.Where(&ProxyTableConfig{DatabaseName: defaultDbName}).Count(&total)
	if total == 0 {
		var default_param params.ProxyTableConfigParams
		default_param.DatabaseName = defaultDbName
		default_param.TableName = defaultTbName
		default_param.ReadTimeout = 10
		default_param.WriteTimeout = 10
		default_param.AllowedFlow = 100
		proxy_config.Store(default_param)
	}
	if isTotal {
		db.Where(&ProxyTableConfig{DatabaseName: para.DatabaseName}).Count(&total)
	}
	if para.Page > 0 && para.Limit > 0 {
		offset := (para.Page - 1) * para.Limit
		db = db.Offset(offset).Limit(para.Limit)
	}
	db.Where(&ProxyTableConfig{DatabaseName: para.DatabaseName}).Find(&proxy_configs)
	return proxy_configs, total
}

func (proxy_config *ProxyTableConfigModel) Update(params params.ProxyTableConfigParams) *common.Status {
	db := proxy_config.ctx.Db()
	if status := proxy_config.checkDupName(params.DatabaseName, params.TableName); status.Code() != common.TableNameDup {
		return common.StatusError(common.TableNameDup)
	}

	if err := db.Model(&ProxyTableConfig{Id: params.Id}).UpdateColumns(ProxyTableConfig{
		ReadTimeout:  params.ReadTimeout,
		WriteTimeout: params.WriteTimeout,
		AllowedFlow: params.AllowedFlow,
	}).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (proxy_config *ProxyTableConfigModel) checkDupName(DatabaseName string, tableName string) *common.Status {
	var count int
	db := proxy_config.ctx.Db().Model(&ProxyTableConfig{})
	if err := db.Model(&ProxyTableConfig{}).Where(&ProxyTableConfig{DatabaseName: DatabaseName, TableName: tableName}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if count != 0 {
		return common.StatusError(common.TableNameDup)
	}
	return common.StatusOk()
}
