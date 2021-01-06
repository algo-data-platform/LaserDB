package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type MachineCategory struct {
	Id        uint32 `gorm:"primary_key"`
	Name      string `gorm:"type:varchar(255);not null;unique_index"`
	Desc      string `gorm:"type:varchar(256);"`
	CreatedAt time.Time
	Machines  []Machine `gorm:"ForeignKey:CategoryId"`
}

type MachineCategoryModel struct {
	ctx *context.Context
}

func NewMachineCategoryModel(ctx *context.Context) *MachineCategoryModel {
	instance := &MachineCategoryModel{
		ctx: ctx,
	}
	return instance
}

func (db *MachineCategoryModel) Store(params params.MachineCategoryParams) *common.Status {
	dbModel := db.ctx.Db()
	if status := db.CheckDupName(params.Name); !status.Ok() {
		return status
	}

	machineCategoryAdd := &MachineCategory{
		Name:      params.Name,
		Desc:      params.Desc,
		CreatedAt: time.Now(),
	}

	if err := dbModel.Create(machineCategoryAdd).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (db *MachineCategoryModel) List(params params.MachineCategoryListParams, isTotal bool) ([]MachineCategory, uint32) {
	dbModel := db.ctx.Db().Model(&MachineCategory{})

	var total uint32
	var machineCategory []MachineCategory
	if isTotal {
		dbModel.Count(&total)
	}

	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		dbModel = dbModel.Offset(offset).Limit(params.Limit)
	}
	dbModel.Preload("Machines").Find(&machineCategory)

	return machineCategory, total
}

func (db *MachineCategoryModel) Update(params params.MachineCategoryParams) *common.Status {
	dbModel := db.ctx.Db()

	if status := db.CheckDupName(params.Name); status.Code() != common.MachineCategoryDup {
		return common.StatusError(common.MachineCategoryNotExists)
	}

	if err := dbModel.Model(&MachineCategory{Id: params.Id}).UpdateColumns(MachineCategory{
		Desc: params.Desc,
	}).Error; err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (db *MachineCategoryModel) Delete(params params.MachineCategoryDeleteParams) *common.Status {
	dbModel := db.ctx.Db()
	tx := dbModel.Begin()

	var categoryIdCount uint32
	if err := dbModel.Model(&MachineCategory{}).Where(&MachineCategory{Id: params.Id}).Count(&categoryIdCount).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	if categoryIdCount == 0 {
		tx.Rollback()
		return common.StatusError(common.MachineCategoryNotExists)
	}

	var machineNumber uint32
	tx.Table("machine_categories").Where("machine_categories.id = ?", params.Id).
		Joins("Join machines on machines.category_id = machine_categories.id").Count(&machineNumber)
	if machineNumber > 0 {
		tx.Rollback()
		return common.StatusError(common.MachineCategoryDeleteWhenMachineExists)
	}

	if err := dbModel.Delete(&MachineCategory{Id: params.Id}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}

	tx.Commit()
	return common.StatusOk()
}

func (db *MachineCategoryModel) CheckDupName(name string) *common.Status {
	var count int
	dbModel := db.ctx.Db().Model(&MachineCategory{})
	if err := dbModel.Model(&MachineCategory{}).Where(&MachineCategory{Name: name}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}

	if count != 0 {
		return common.StatusError(common.MachineCategoryDup)
	}

	return common.StatusOk()
}
