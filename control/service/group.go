package service

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"time"
)

type Group struct {
	Id           uint32     `gorm:"primary_key"`
	Name         string     `gorm:"type:varchar(255);not null;unique_index"`
	Alias        string     `gorm:"type:varchar(255);not null"`
	NodeConfigId uint32     `gorm:"type:int(32);not null"`
	Config       NodeConfig `gorm:"ForeignKey:NodeConfigId"`
	Desc         string     `gorm:"type:varchar(256);"`
	Nodes        []Node     `gorm:"ForeignKey:GroupId;AssociationForeignKey:Id"`
	ClusterId    uint32     `gorm:"type:int(32);not null"`
	ClusterInfo  Cluster    `gorm:"ForeignKey:ClusterId"`
	DcId         uint32     `gorm:"type:int(32);not null"`
	Dc           Dc         `gorm:"ForeignKey:DcId;AssociationForeignKey:Id"`
	CreatedAt    time.Time
}
type GroupChangeCallback func() *common.Status

type GroupModel struct {
	ctx           *context.Context
	groupChangeCb GroupChangeCallback
}

func NewGroupModel(ctx *context.Context) *GroupModel {
	instance := &GroupModel{
		ctx:           ctx,
		groupChangeCb: nil,
	}
	return instance
}

func (group *GroupModel) SubscribeGroupChange(cb GroupChangeCallback) {
	group.groupChangeCb = cb
}

func (group *GroupModel) Store(params params.GroupParams) *common.Status {
	db := group.ctx.Db()

	if status := group.checkDupName(params.Name); !status.Ok() {
		return status
	}
	groupInfo := &Group{
		Name:         params.Name,
		Alias:        params.Alias,
		NodeConfigId: params.NodeConfigId,
		Desc:         params.Desc,
		DcId:         params.DcId,
		ClusterId:    params.ClusterId,
		CreatedAt:    time.Now(),
	}
	if err := db.Create(groupInfo).Error; err != nil {
		return common.StatusWithError(err)
	}

	if group.groupChangeCb != nil {
		group.groupChangeCb()
	}
	return common.StatusOk()
}

func (group *GroupModel) List(params params.GroupListParams, isTotal bool) ([]Group, uint32) {
	db := group.ctx.Db().Model(&Group{}).Where(&Group{
		Id:        params.GroupId,
		Name:      params.GroupName,
		ClusterId: params.ClusterId,
	})

	var total uint32
	var groups []Group
	if isTotal {
		db.Count(&total)
	}

	if params.Page > 0 && params.Limit > 0 {
		offset := (params.Page - 1) * params.Limit
		db = db.Offset(offset).Limit(params.Limit)
	}
	if params.ShowDetails {
		db.Preload("Config").Preload("Nodes").Preload("ClusterInfo").Preload("Dc").Find(&groups)
	} else {
		db.Preload("Dc").Find(&groups)
	}
	return groups, total
}

func (group *GroupModel) Update(params params.GroupParams) *common.Status {
	db := group.ctx.Db()

	if status := group.checkDupName(params.Name); status.Code() != common.GroupNameDup {
		return common.StatusError(common.GroupNotExist)
	}
	if err := db.Model(&Group{Id: params.Id}).UpdateColumns(Group{
		Alias:        params.Alias,
		NodeConfigId: params.NodeConfigId,
		Desc:         params.Desc,
		// DcId:         params.DcId,
	}).Error; err != nil {
		return common.StatusWithError(err)
	}
	if group.groupChangeCb != nil {
		group.groupChangeCb()
	}
	return common.StatusOk()
}

func (group *GroupModel) Delete(params params.GroupDeleteParams) *common.Status {
	dbModel := group.ctx.Db()
	tx := dbModel.Begin()

	var usedCount uint32
	tx.Table("groups").Where("groups.id = ?", params.Id).
		Joins("Join nodes on nodes.group_id = groups.id").Count(&usedCount)
	if usedCount > 0 {
		tx.Rollback()
		return common.StatusError(common.GroupInUse)
	}

	if err := group.ctx.Db().Delete(&Group{Id: params.Id}).Error; err != nil {
		tx.Rollback()
		return common.StatusWithError(err)
	}
	tx.Commit()
	return common.StatusOk()
}

func (group *GroupModel) checkDupName(groupName string) *common.Status {
	var count int
	db := group.ctx.Db().Model(&Group{})
	if err := db.Where(&Group{Name: groupName}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if count != 0 {
		return common.StatusError(common.GroupNameDup)
	}
	return common.StatusOk()
}
