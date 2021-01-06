package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

// LaserDatabase struct define fields for table laser::LaserDatabase
type LaserDatabase struct {
	Id        uint32 `gorm:"primary_key"`
	Name      string `gorm:"type:varchar(255);not null;unique"`
	Desc      string `gorm:"type:varchar(256);not null;"`
	CreatedAt time.Time
}

// LaserDatabaseModel definition
type LaserDatabaseModel struct {
	ctx *context.Context
}

// NewLaserDatabaseModel return a instance of DatabaseModel
func NewLaserDatabaseModel(ctx *context.Context) *LaserDatabaseModel {
	instance := &LaserDatabaseModel{
		ctx: ctx,
	}
	return instance
}

func (db *LaserDatabaseModel) Store(params params.DatabaseAddParams) *common.Status {
	dbMod := db.ctx.Db()

	if status := db.CheckDupName(params.Name); !status.Ok() {
		return status
	}

	dbAdd := &LaserDatabase{
		Name:      params.Name,
		Desc:      params.Desc,
		CreatedAt: time.Now(),
	}

	if err := dbMod.Create(dbAdd).Error; err != nil {
		return common.StatusWithError(err)
	}

	return common.StatusOk()
}

func (db *LaserDatabaseModel) List(params params.DatabaseListParams, isTotal bool) ([]LaserDatabase, uint32) {
	dbMod := db.ctx.Db().Model(&LaserDatabase{})

	var total uint32
	var databases []LaserDatabase
	if isTotal {
		dbMod.Count(&total)
	}

	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		dbMod = dbMod.Offset(offset).Limit(params.Limit)
	}
	dbMod.Find(&databases)

	return databases, total
}

func (db *LaserDatabaseModel) Update(param params.DatabaseUpdateParams) *common.Status {
	dbMod := db.ctx.Db()

	if status := db.CheckDupName(param.Name); status.Code() != common.DatabaseNameDup {
		return common.StatusError(common.DatabaseNotExists)
	}

	if err := dbMod.Model(&LaserDatabase{Id: param.Id}).UpdateColumns(LaserDatabase{
		Desc: param.Desc,
	}).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (db *LaserDatabaseModel) Delete(params params.DatabaseDeleteParams) *common.Status {
	dbModel := db.ctx.Db()
	tx := dbModel.Begin()

	var usedCount uint32
	tx.Table("laser_databases").Where("laser_databases.id = ?", params.Id).
		Joins("Join tables on tables.database_id = laser_databases.id").Count(&usedCount)
	if usedCount > 0 {
		tx.Rollback()
		return common.StatusError(common.DatabaseInUse)
	}
	if err := db.ctx.Db().Delete(&LaserDatabase{Id: params.Id}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (db *LaserDatabaseModel) CheckDupName(databaseName string) *common.Status {
	var count int
	dbMod := db.ctx.Db().Model(&LaserDatabase{})
	if err := dbMod.Model(&LaserDatabase{}).Where(&LaserDatabase{Name: databaseName}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}

	if count != 0 {
		return common.StatusError(common.DatabaseNameDup)
	}

	return common.StatusOk()
}

func (db *LaserDatabaseModel) GetIdByName(databaseName string) uint32 {
	var dataBaseData LaserDatabase
	db.ctx.Db().Find(&dataBaseData, &LaserDatabase{Name: databaseName})

	return dataBaseData.Id
}
