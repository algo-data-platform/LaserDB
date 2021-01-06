package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

const (
	TableDisabled uint32 = 0
	TableEnabled  uint32 = 1
)

const (
	SingleOperation uint32 = iota + 1
	MultipleOperation
)

type TrafficRestrictionType uint32

const (
	KPS TrafficRestrictionType = 0
	QPS TrafficRestrictionType = 1
)

type TrafficRestrictionLimit struct {
	Id            uint32  `gorm:"primary_key"`
	Name          string  `gorm:"type:varchar(255);not null;unique_index:name_table_id"`
	OperationType uint32  `gorm:"type:int(32);not null"` // 1-SingleOperation 2-MultipleOperation
	LimitType     *uint32 `gorm:"type:int(32);not null"` // 0-KPS 1-QPS
	LimitValue    uint32  `gorm:"type:int(32);not null"`
	TableId       uint32  `gorm:"type:int(32);unique_index:name_table_id"`
}

type Table struct {
	Id              uint32                    `gorm:"primary_key"`
	Name            string                    `gorm:"type:varchar(255);not null"`
	Status          *uint32                   `gorm:"type:int(32);not null;default 1"` // 0-禁用，1-启用
	DenyAll         *uint32                   `gorm:"type:int(32);not null;default 0"` // 0-不开启，1-开启
	PartitionNumber uint32                    `gorm:"type:int(32);not null;"`
	Ttl             *uint64                   `gorm:"type:bigint unsigned;not null;default 0"`
	Desc            string                    `gorm:"type:varchar(256);"`
	DcId            uint32                    `gorm:"type:int(32);not null"`
	Dc              Dc                        `gorm:"ForeignKey:DcId;AssociationForeignKey:Id"`
	DistDcId        uint32                    `gorm:"type:int(32);not null"`
	DistDc          Dc                        `gorm:"ForeignKey:DistDcId;AssociationForeignKey:Id"`
	DatabaseId      uint32                    `gorm:"type:int(32);not null"`
	ConfigId        uint32                    `sql:"type:int(32);not null"`
	Database        LaserDatabase             `gorm:"ForeignKey:DatabaseId"`
	Config          TableConfig               `gorm:"ForeignKey:ConfigId"`
	EdgeFlowRatio   *uint32                   `gorm:"type:int(32);not null;"`
	TrafficLimits   []TrafficRestrictionLimit `gorm:"ForeignKey:TableId;AssociationForeignKey:Id"`
	BindEdgeNodes   []Node                    `gorm:"many2many:table_edge_nodes"`
	CreatedAt       time.Time
}

type TableModel struct {
	ctx *context.Context
}

func NewTableModel(ctx *context.Context) *TableModel {
	instance := &TableModel{
		ctx: ctx,
	}
	return instance
}

func (table *TableModel) Store(params params.TableAddParams) *common.Status {
	db := table.ctx.Db()

	if status := table.checkDupName(params.DatabaseId, params.Name); !status.Ok() {
		return status
	}

	trafficLimits := make([]TrafficRestrictionLimit, 0)
	for _, item := range params.TrafficLimits {
		trafficLimit := TrafficRestrictionLimit{
			Name:          item.Name,
			OperationType: item.OperationType,
			LimitType:     item.LimitType,
			LimitValue:    item.LimitValue,
		}
		trafficLimits = append(trafficLimits, trafficLimit)
	}

	bindEdgeNodes := make([]Node, 0)
	for _, id := range params.BindEdgeNodeIds {
		node := Node{
			Id: id,
		}
		bindEdgeNodes = append(bindEdgeNodes, node)
	}

	tableAdd := &Table{
		Name:            params.Name,
		Status:          params.Status,
		DenyAll:         params.DenyAll,
		PartitionNumber: params.PartitionNumber,
		Ttl:             params.Ttl,
		Desc:            params.Desc,
		DcId:            params.DcId,
		DistDcId:        params.DistDcId,
		DatabaseId:      params.DatabaseId,
		ConfigId:        params.ConfigId,
		EdgeFlowRatio:   params.EdgeFlowRatio,
		TrafficLimits:   trafficLimits,
		CreatedAt:       time.Now(),
	}
	tx := db.Begin()
	if err := tx.Create(tableAdd).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}

	if err := tx.Model(tableAdd).Association("BindEdgeNodes").Append(&bindEdgeNodes).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (table *TableModel) List(params params.TableListParams, isTotal bool) ([]Table, uint32) {
	db := table.ctx.Db().Model(&Table{}).Where("tables.name like ?", "%"+params.TableName+"%").
		Joins("Join laser_databases on tables.database_id = laser_databases.id AND laser_databases.name like ?", "%"+params.DatabaseName+"%")

	if params.TableId > 0 {
		db = db.Where(&Table{Id: params.TableId})
	}

	if params.DatabaseId > 0 {
		db = db.Where(&Table{DatabaseId: params.DatabaseId})
	}

	var total uint32
	var tables []Table
	if isTotal {
		db.Count(&total)
	}
	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		db = db.Offset(offset).Limit(params.Limit)
	}
	db.Preload("Dc").Preload("DistDc").Preload("Database").Preload("Config").Find(&tables)
	return tables, total
}

func (table *TableModel) ListDetail(params params.TableListParams, isTotal bool) ([]Table, uint32) {
	db := table.ctx.Db().Model(&Table{}).Where(&Table{Id: params.TableId, DatabaseId: params.DatabaseId})

	var total uint32
	var tables []Table
	if isTotal {
		db.Count(&total)
	}
	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		db = db.Offset(offset).Limit(params.Limit)
	}
	db.Preload("Dc").Preload("DistDc").Preload("Database").Preload("Config").Preload("TrafficLimits").Preload("BindEdgeNodes").Find(&tables)
	return tables, total
}

func (table *TableModel) Update(params params.TableUpdateParams) *common.Status {
	db := table.ctx.Db()

	if status := table.checkDupName(params.DatabaseId, params.Name); status.Code() != common.TableNameDup {
		return common.StatusError(common.TableNotExists)
	}

	trafficLimits := make([]TrafficRestrictionLimit, 0)
	for _, item := range params.TrafficLimits {
		trafficLimit := TrafficRestrictionLimit{
			Name:          item.Name,
			OperationType: item.OperationType,
			LimitType:     item.LimitType,
			LimitValue:    item.LimitValue,
		}
		trafficLimits = append(trafficLimits, trafficLimit)
	}

	bindEdgeNodes := make([]Node, 0)
	for _, id := range params.BindEdgeNodeIds {
		node := Node{
			Id: id,
		}
		bindEdgeNodes = append(bindEdgeNodes, node)
	}

	tx := db.Begin()
	if err := tx.Model(&Table{Id: params.Id}).Association("TrafficLimits").Clear().Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Model(&Table{Id: params.Id}).Association("TrafficLimits").Replace(trafficLimits).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}

	if err := tx.Unscoped().Where("table_id = 0 or table_id is NULL").Delete(&TrafficRestrictionLimit{}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Model(&Table{Id: params.Id}).Association("BindEdgeNodes").Replace(&bindEdgeNodes).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Model(&Table{Id: params.Id}).UpdateColumns(Table{
		Status:        params.Status,
		DenyAll:       params.DenyAll,
		Ttl:           params.Ttl,
		Desc:          params.Desc,
		DcId:          params.DcId,
		DistDcId:      params.DistDcId,
		ConfigId:      params.ConfigId,
		EdgeFlowRatio: params.EdgeFlowRatio,
	}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}

	tx.Commit()
	return common.StatusOk()
}

func (table *TableModel) Delete(params params.TableDeleteParams) *common.Status {
	db := table.ctx.Db()
	tx := db.Begin()
	if err := tx.Model(&Table{Id: params.Id}).Association("TrafficLimits").Clear().Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Unscoped().Where("table_id = 0 or table_id is NULL").Delete(&TrafficRestrictionLimit{}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := tx.Model(&Table{Id: params.Id}).Association("BindEdgeNodes").Clear().Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if err := db.Delete(&Table{Id: params.Id}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (table *TableModel) checkDupName(databaseId uint32, tableName string) *common.Status {
	var count int
	db := table.ctx.Db().Model(&Table{})
	if err := db.Model(&Table{}).Where(&Table{DatabaseId: databaseId, Name: tableName}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if count != 0 {
		return common.StatusError(common.TableNameDup)
	}
	return common.StatusOk()
}

func (table *TableModel) FindOne(databaseId uint32, tableName string) Table {
	var tableData Table
	table.ctx.Db().Find(&tableData, &Table{DatabaseId: databaseId, Name: tableName})
	return tableData
}
