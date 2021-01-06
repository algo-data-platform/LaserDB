package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"net"
	"strings"
	"time"
)

type Machine struct {
	Id              uint32 `gorm:"primary_key"`
	CategoryId      uint32 `gorm:"type:int(10);not null"`
	Ip              string `gorm:"type:varchar(255);not null;unique_index"`
	CpuCoreNumber   string `gorm:"type:varchar(255);not null"`
	MemorySizeGb    string `gorm:"type:varchar(255);not null"`
	Desc            string `gorm:"type:varchar(256);"`
	CreatedAt       time.Time
	MachineCategory MachineCategory `gorm:"ForeignKey:CategoryId"`
}

type MachineModel struct {
	ctx *context.Context
}

func NewMachineModel(ctx *context.Context) *MachineModel {
	instance := &MachineModel{
		ctx: ctx,
	}
	return instance
}

func (db *MachineModel) Store(params params.MachineParams) *common.Status {
	dbModel := db.ctx.Db()
	ipList := strings.Split(params.Ip, "\n")

	for i, ip := range ipList {
		ipList[i] = strings.TrimSpace(ipList[i])
		address := net.ParseIP(ipList[i])
		if address == nil {
			ipList = append(ipList[:i], ipList[i+1:]...)
		}
		if status := db.CheckDupIp(ip); !status.Ok() {
			return status
		}
	}

	if len(ipList) == 0 {
		return common.StatusOk()
	}

	for _, ip := range ipList {
		machineAdd := &Machine{
			CategoryId:    params.CategoryId,
			Ip:            ip,
			CpuCoreNumber: params.CpuCoreNumber,
			MemorySizeGb:  params.MemorySizeGb,
			Desc:          params.Desc,
			CreatedAt:     time.Now(),
		}

		if err := dbModel.Create(machineAdd).Error; err != nil {
			return common.StatusWithError(err)
		}
	}

	return common.StatusOk()
}

func (db *MachineModel) List(params params.MachineListParams, isTotal bool) ([]Machine, uint32) {
	dbModel := db.ctx.Db().Model(&Machine{})

	var total uint32
	var machine []Machine

	dbModel = dbModel.Where("ip like ?", "%"+params.MachineIp+"%").
		Joins("Join machine_categories on machine_categories.id = machines.category_id AND machine_categories.name like ?", "%"+params.CategoryName+"%").
		Preload("MachineCategory")

	if isTotal {
		dbModel.Count(&total)
	}

	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		dbModel = dbModel.Offset(offset).Limit(params.Limit)
	}

	dbModel.Find(&machine)

	return machine, total
}

func (db *MachineModel) Update(params params.MachineUpdateParams) *common.Status {
	dbModel := db.ctx.Db()

	params.Ip = strings.TrimSpace(params.Ip)
	if status := db.CheckDupIp(params.Ip); status.Code() != common.MachineDup {
		return common.StatusError(common.MachineNotExists)
	}

	if err := dbModel.Model(&Machine{Id: params.Id}).UpdateColumns(Machine{
		CategoryId:    params.CategoryId,
		Ip:            params.Ip,
		CpuCoreNumber: params.CpuCoreNumber,
		MemorySizeGb:  params.MemorySizeGb,
		Desc:          params.Desc,
	}).Error; err != nil {
		return common.StatusWithError(err)
	}

	return common.StatusOk()
}

func (db *MachineModel) Delete(params params.MachineDeleteParams) *common.Status {
	dbModel := db.ctx.Db()
	tx := dbModel.Begin()

	var usedCount uint32
	tx.Table("machines").Where("machines.id = ?", params.Id).
		Joins("Join nodes on nodes.host = machines.ip").Count(&usedCount)
	if usedCount > 0 {
		tx.Rollback()
		return common.StatusError(common.MachineInUse)
	}

	if err := dbModel.Delete(&Machine{Id: params.Id}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (db *MachineModel) CheckDupIp(ip string) *common.Status {
	var count int
	dbModel := db.ctx.Db().Model(&Machine{})
	if err := dbModel.Model(&Machine{}).Where(&Machine{Ip: ip}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}

	if count != 0 {
		return common.StatusError(common.MachineDup)
	}

	return common.StatusOk()
}
