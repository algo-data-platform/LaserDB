package service

import (
	"bytes"
	"fmt"
	"html/template"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"os"
	"path/filepath"
	"strings"
	"time"
)

const (
	WaitForUpdate uint32 = iota + 1
	WaitForProcess
	TicketReject
	ProcessFailed
	ProcessSucceed
)

const (
	TicketStatusWaitForCheck   string = "待审核"
	TicketHasBeenReject        string = "被驳回"
	TicketStatusProcessResult  string = "录入结果"
	CreateDatabaseFailed       string = "创建数据库失败"
	CreateTableFailed          string = "创建数据表失败"
	TicketStatusProcessSucceed string = "录入成功"
)

const (
	SubjectToCreator string = "Laser存储工单处理结果"
	SubjectToHandler string = "Laser存储工单申请"
)

const (
	EmailTemplateToCreator string = "templates/email-to-creator.html"
	EmailTemplateToHandler string = "templates/email-to-handler.html"
)

const EmailTail string = "@gmail.com"

type Ticket struct {
	Id                       uint32  `gorm:"primary_key"`
	Creator                  string  `gorm:"type:varchar(255);not null"`
	SDKType                  string  `gorm:"type:varchar(255);not null"`
	ReadContactPersion       string  `gorm:"type:varchar(255);not null"`
	WriteContactPersion      string  `gorm:"type:varchar(255);not null"`
	ImportDataContactPersion string  `gorm:"type:varchar(255);not null"`
	BusinessLine             string  `gorm:"type:varchar(255);not null"`
	BusinessDescription      string  `gorm:"type:varchar(255);not null"`
	BusinessKR               string  `gorm:"type:varchar(255);not null"`
	DatabaseName             string  `gorm:"type:varchar(255);not null"`
	TableName                string  `gorm:"type:varchar(255);not null"`
	ReadQPS                  uint32  `gorm:"type:int(32);not null"`
	WriteQPS                 uint32  `gorm:"type:int(32);not null"`
	RequestDelayLimit        uint32  `gorm:"type:int(32);not null"`
	DataExpirationTime       uint64  `gorm:"type:bigint;not null"`
	DataSize                 uint32  `gorm:"type:int(32);not null"`
	ValueSize                uint32  `gorm:"type:int(32);not null"`
	CrashInfluence           string  `gorm:"type:varchar(255);not null"`
	Command                  string  `gorm:"type:varchar(255);not null"`
	KeyNum                   uint32  `gorm:"type:int(32);not null"`
	DockingPersonnel         string  `gorm:"type:varchar(255);not null"`
	Status                   *uint32 `gorm:"type:int(32);not null"`
	PartitionNum             *uint32 `gorm:"type:int(32);not null"`
	CreatedAt                time.Time
}

type TicketMailInfo struct {
	Creator            string
	Handler            string
	MailTo             []string
	CurrentStatus      uint32
	CurrentStr         string
	DatabaseName       string
	TableName          string
	TableConfigName    string
	PartitionNum       uint32
	DataExpirationTime uint64
}

type TicketModel struct {
	ctx *context.Context
}

func NewTicketModel(ctx *context.Context) *TicketModel {
	instance := &TicketModel{
		ctx: ctx,
	}
	return instance
}

func ConvStrArrayToStr(value []string) string {
	var ret string
	for _, v := range value {
		ret += v + ","
	}
	return ret
}

func ConvStrToStrArray(cmd string) []string {
	array_str := strings.Split(cmd, ",")
	ret := make([]string, 0)
	for _, str := range array_str {
		if str == "" {
			continue
		}
		ret = append(ret, str)
	}
	return ret
}

func initTicket(param params.TicketParams) Ticket {
	ticket := Ticket{
		Id:                       param.Id,
		Creator:                  param.Creator,
		SDKType:                  ConvStrArrayToStr(param.SDKType),
		ReadContactPersion:       param.ReadContactPersion,
		WriteContactPersion:      param.WriteContactPersion,
		ImportDataContactPersion: param.WriteContactPersion,
		BusinessLine:             param.BusinessLine,
		BusinessDescription:      param.BusinessDescription,
		BusinessKR:               param.BusinessKR,
		DatabaseName:             param.DatabaseName,
		TableName:                param.TableName,
		ReadQPS:                  param.ReadQPS,
		WriteQPS:                 param.WriteQPS,
		RequestDelayLimit:        param.RequestDelayLimit,
		DataExpirationTime:       param.DataExpirationTime,
		DataSize:                 param.DataSize,
		ValueSize:                param.ValueSize,
		CrashInfluence:           param.CrashInfluence,
		Command:                  ConvStrArrayToStr(param.Command),
		KeyNum:                   param.KeyNum,
		DockingPersonnel:         param.DockingPersonnel,
		PartitionNum:             param.PartitionNum,
		Status:                   param.Status,
		CreatedAt:                time.Now(),
	}

	return ticket
}

func initParamTicket(param Ticket) params.TicketParams {
	ticket := params.TicketParams{
		Id:                       param.Id,
		Creator:                  param.Creator,
		SDKType:                  ConvStrToStrArray(param.SDKType),
		ReadContactPersion:       param.ReadContactPersion,
		WriteContactPersion:      param.WriteContactPersion,
		ImportDataContactPersion: param.WriteContactPersion,
		BusinessLine:             param.BusinessLine,
		BusinessDescription:      param.BusinessDescription,
		BusinessKR:               param.BusinessKR,
		DatabaseName:             param.DatabaseName,
		TableName:                param.TableName,
		ReadQPS:                  param.ReadQPS,
		WriteQPS:                 param.WriteQPS,
		RequestDelayLimit:        param.RequestDelayLimit,
		DataExpirationTime:       param.DataExpirationTime,
		DataSize:                 param.DataSize,
		ValueSize:                param.ValueSize,
		CrashInfluence:           param.CrashInfluence,
		Command:                  ConvStrToStrArray(param.Command),
		KeyNum:                   param.KeyNum,
		DockingPersonnel:         param.DockingPersonnel,
		PartitionNum:             param.PartitionNum,
		Status:                   param.Status,
	}

	return ticket
}

func (ticket *TicketModel) Store(param params.TicketParams) *common.Status {
	if status := ticket.CheckDupName(param.DatabaseName, param.TableName); !status.Ok() {
		return status
	}

	ticketAdd := initTicket(param)
	dbMod := ticket.ctx.Db()
	if err := dbMod.Create(&ticketAdd).Error; err != nil {
		return common.StatusWithError(err)
	}
	if *param.Status == WaitForProcess {
		info := TicketMailInfo{
			Creator:            param.Creator,
			CurrentStatus:      *param.Status,
			DatabaseName:       param.DatabaseName,
			TableName:          param.TableName,
			DataExpirationTime: param.DataExpirationTime,
			CurrentStr:         TicketStatusWaitForCheck,
		}
		ticket.SendMail(info)
	}

	return common.StatusOk()
}

func (ticket *TicketModel) List(param params.TicketListParams, isTotal bool) ([]params.TicketParams, uint32) {
	ticketMod := ticket.ctx.Db().Model(&Ticket{})
	var total uint32
	var ticksets []Ticket
	retickets := make([]params.TicketParams, 0)
	if isTotal {
		ticketMod.Count(&total)
	}

	if param.Page > 0 && param.Limit > 0 {
		offset := (param.Page - 1) * param.Limit
		ticketMod = ticketMod.Offset(offset).Limit(param.Limit)
	}

	ticketMod.Find(&ticksets)
	for _, ticket := range ticksets {
		t := initParamTicket(ticket)
		retickets = append(retickets, t)
	}
	return retickets, total
}

func (ticket *TicketModel) Update(param params.TicketParams) *common.Status {
	ticketMod := ticket.ctx.Db()
	if status := ticket.CheckDupName(param.DatabaseName, param.TableName); status.Code() != common.TicketDup {
		return common.StatusError(common.TicketNotExists)
	}

	ticketUpd := initTicket(param)
	if err := ticketMod.Model(&Ticket{Id: param.Id}).UpdateColumns(ticketUpd).Error; err != nil {
		return common.StatusWithError(err)
	}

	if *param.Status == WaitForProcess {
		info := TicketMailInfo{
			Creator:            param.Creator,
			CurrentStatus:      *param.Status,
			DatabaseName:       param.DatabaseName,
			TableName:          param.TableName,
			DataExpirationTime: param.DataExpirationTime,
			CurrentStr:         TicketStatusWaitForCheck,
		}
		ticket.SendMail(info)
	}

	return common.StatusOk()
}

func (ticket *TicketModel) Delete(params params.TicketDeleteParams) *common.Status {
	if err := ticket.ctx.Db().Delete(&Ticket{Id: params.Id}).Error; err != nil {
		return common.StatusWithError(err)
	}

	return common.StatusOk()
}

func (ticket *TicketModel) initMailTo(res params.TicketParams, mail *[]string) {
	dockingPersonnel := ConvStrToStrArray(res.DockingPersonnel)
	for _, p := range dockingPersonnel {
		ticket.addToArray(mail, p)
	}
	ticket.addToArray(mail, res.Creator)
	ticket.addToArray(mail, res.ReadContactPersion)
	ticket.addToArray(mail, res.WriteContactPersion)
	ticket.addToArray(mail, res.ImportDataContactPersion)
}

func (ticket *TicketModel) addToArray(persion_list *[]string, to string) {
	if to == "" {
		return
	}
	var flag bool = false
	for _, p := range *persion_list {
		if p == to {
			flag = true
			break
		}
	}
	if !flag {
		*persion_list = append(*persion_list, to)
	}
}

func (ticket *TicketModel) Process(param params.TicketProcessParams) *common.Status {
	var access_process bool = false
	admin_users := ticket.ctx.GetConfig().AdminUsers()
	for _, user := range admin_users {
		if param.Handler == user {
			access_process = true
			break
		}
	}

	if access_process == false {
		ticket.ctx.Log().Info(fmt.Sprint("当前登录用户无权限审核工单"))
		return common.StatusError(common.TicketCanotHandle)
	}

	if *param.TicketAcceptStatus == 0 {
		ticket.reject(param)
	} else {
		ticket.accept(param)
	}
	return common.StatusOk()
}

func (ticket *TicketModel) reject(param params.TicketProcessParams) {
	res := initParamTicket(ticket.GetSpecTicket(param.DatabaseName, param.TableName))
	info := TicketMailInfo{
		Handler:      param.Handler,
		DatabaseName: param.DatabaseName,
		TableName:    param.TableName,
	}
	info.Creator = res.Creator
	info.CurrentStatus = TicketReject
	info.CurrentStr = TicketStatusProcessResult + " : " + TicketHasBeenReject + ", " + param.RejectReason
	ticket.initMailTo(res, &info.MailTo)
	ticket.SendMail(info)

	*res.Status = WaitForUpdate
	ticket.Update(res)
}

func (ticket *TicketModel) accept(param params.TicketProcessParams) *common.Status {
	info := TicketMailInfo{
		Handler:            param.Handler,
		DatabaseName:       param.DatabaseName,
		TableName:          param.TableName,
		DataExpirationTime: *param.DataExpirationTime,
		TableConfigName:    param.TableConfigName,
		PartitionNum:       *param.PartitionNum,
	}
	// create database
	status, dataBaseId := ticket.createDatabase(param.DatabaseName)
	res := initParamTicket(ticket.GetSpecTicket(param.DatabaseName, param.TableName))
	if status.Code() != common.OK {
		info.Creator = res.Creator
		info.CurrentStatus = ProcessFailed
		info.CurrentStr = TicketStatusProcessResult + " : " + CreateDatabaseFailed
		ticket.SendMail(info)
		return status
	}
	// create table
	status = ticket.createTable(dataBaseId, param)
	if status.Code() != common.OK {
		info.Creator = res.Creator
		info.CurrentStatus = ProcessFailed
		info.CurrentStr = TicketStatusProcessResult + " : " + CreateTableFailed
		ticket.SendMail(info)
		return status
	}
	res.PartitionNum = param.PartitionNum
	*res.Status = ProcessSucceed

	ticket.Update(res)
	info.Creator = res.Creator
	info.CurrentStatus = ProcessSucceed
	info.CurrentStr = TicketStatusProcessResult + " : " + TicketStatusProcessSucceed
	info.MailTo = make([]string, 0)
	ticket.initMailTo(res, &info.MailTo)
	ticket.SendMail(info)

	return common.StatusOk()
}

func (ticket *TicketModel) createDatabase(databaseName string) (*common.Status, uint32) {
	var dataBaseId uint32
	dbMod := NewLaserDatabaseModel(ticket.ctx)
	dataBaseParams := params.DatabaseAddParams{
		Name: databaseName,
	}
	status := dbMod.Store(dataBaseParams)
	if status.Code() == common.DatabaseNameDup {
		ticket.ctx.Log().Info(fmt.Sprint("数据库已存在"))
		status = common.StatusOk()
	}
	dataBaseId = dbMod.GetIdByName(databaseName)

	return status, dataBaseId
}

func (ticket *TicketModel) createTable(databaseId uint32, param params.TicketProcessParams) *common.Status {
	var tableStatus uint32 = TableEnabled
	var tableDenyAll uint32 = 0
	var edgeFlowRatio uint32 = 0
	var tableTtl uint64 = *param.DataExpirationTime
	tableMod := NewTableModel(ticket.ctx)
	tableParams := params.TableAddParams{
		Name:            param.TableName,
		DcId:            param.DcId,
		DistDcId:        param.DcId,
		Status:          &tableStatus,
		DenyAll:         &tableDenyAll,
		PartitionNumber: *param.PartitionNum,
		Ttl:             &tableTtl,
		DatabaseId:      databaseId,
		ConfigId:        *param.TableConfigId,
		EdgeFlowRatio:   &edgeFlowRatio,
	}
	status := tableMod.Store(tableParams)
	if status.Code() == common.TableNameDup {
		// 数据表已存在
		ticket.ctx.Log().Info(fmt.Sprint("数据表已存在"))
		return status
	}

	return status
}

func (ticket *TicketModel) alreadyExist(databaseName string, tableName string) bool {
	dbMod := NewLaserDatabaseModel(ticket.ctx)

	if status := dbMod.CheckDupName(databaseName); status.Code() != common.DatabaseNameDup {
		return false
	}
	dataBaseId := dbMod.GetIdByName(databaseName)
	tableMod := NewTableModel(ticket.ctx)
	if status := tableMod.checkDupName(dataBaseId, tableName); status.Code() == common.TableNameDup {
		return true
	} else {
		return false
	}

}

func (ticket *TicketModel) CheckDupName(databaseName string, tableName string) *common.Status {
	var count int
	db := ticket.ctx.Db().Model(&Ticket{})
	if err := db.Model(&Ticket{}).Where(&Ticket{DatabaseName: databaseName, TableName: tableName}).Count(&count).Error; err != nil {
		return common.StatusWithError(err)
	}
	if count != 0 {
		return common.StatusError(common.TicketDup)
	}
	if ticket.alreadyExist(databaseName, tableName) {
		return common.StatusError(common.TicketDup)
	}

	return common.StatusOk()
}

func (ticket *TicketModel) GetSpecTicket(databaseName string, tableName string) Ticket {
	ticketMod := ticket.ctx.Db().Model(&Ticket{})
	var ret Ticket

	ticketMod.Find(&ret, &Ticket{DatabaseName: databaseName, TableName: tableName})

	return ret
}

func (ticket *TicketModel) SendMail(info TicketMailInfo) {
	var subject string
	user := ticket.ctx.GetConfig().EmailUser()
	passwd := ticket.ctx.GetConfig().EailPassword()
	snmpAdd := ticket.ctx.GetConfig().EmailSnmpAddress()
	from := ticket.ctx.GetConfig().EmailFrom()
	admin_users := ticket.ctx.GetConfig().AdminUsers()
	auth := common.NewLoginAuth(user, passwd)
	recipients := make([]string, 0)
	var t *template.Template
	current_dir, _ := filepath.Abs(filepath.Dir(os.Args[0]))
	creator := current_dir + "/" + EmailTemplateToCreator
	handler := current_dir + "/" + EmailTemplateToHandler
	if (info.CurrentStatus == ProcessSucceed) || (info.CurrentStatus == TicketReject) {
		subject = SubjectToCreator
		for _, user := range info.MailTo {
			address := user + EmailTail
			recipients = append(recipients, address)
		}
		t, _ = template.ParseFiles(creator)
	} else {
		subject = SubjectToHandler
		for _, user := range admin_users {
			address := user + EmailTail
			recipients = append(recipients, address)
		}
		t, _ = template.ParseFiles(handler)
	}

	var body bytes.Buffer
	t.Execute(&body, info)
	err := common.SendMail(snmpAdd, auth, from, recipients, subject, body)
	if err != nil {
		ticket.ctx.Log().Info(fmt.Sprint("Send Mail fail, err : ", err.Error()))
	}
}
