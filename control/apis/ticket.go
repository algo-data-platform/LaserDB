package apis

import (
	"github.com/gin-gonic/gin"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"
)

// @Summary 添加一个工单
// @Tags ticket
// @Accept  json
// @Produce  json
// @Param data body params.TicketParams true "工单信息"
// @Success 200 {object} apis.Result
// @Router /ticket/store [post]
func TicketStcore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.TicketParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		// insert a new record
		ticket := service.NewTicketModel(ctx)
		status := ticket.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询所有工单
// @Tags ticket
// @Accept  json
// @Produce  json
// @Param Loader query string true "登录用户"
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Success 200 {object} apis.Result
// @Router /ticket/list [get]
func TicketList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.TicketListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		ticket := service.NewTicketModel(ctx)
		list, total := ticket.List(param, true)
		result := make([]params.TicketDto, 0)
		for _, info := range list {
			result = append(result, params.TicketDto{
				Id:                       info.Id,
				Creator:                  info.Creator,
				SDKType:                  info.SDKType,
				ReadContactPersion:       info.ReadContactPersion,
				WriteContactPersion:      info.WriteContactPersion,
				ImportDataContactPersion: info.ImportDataContactPersion,
				BusinessLine:             info.BusinessLine,
				BusinessDescription:      info.BusinessDescription,
				BusinessKR:               info.BusinessKR,
				DatabaseName:             info.DatabaseName,
				TableName:                info.TableName,
				ReadQPS:                  info.ReadQPS,
				WriteQPS:                 info.WriteQPS,
				RequestDelayLimit:        info.RequestDelayLimit,
				DataExpirationTime:       info.DataExpirationTime,
				DataSize:                 info.DataSize,
				ValueSize:                info.ValueSize,
				CrashInfluence:           info.CrashInfluence,
				Command:                  info.Command,
				KeyNum:                   info.KeyNum,
				DockingPersonnel:         info.DockingPersonnel,
				PartitionNum:             info.PartitionNum,
				Status:                   info.Status,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新一个工单
// @Tags ticket
// @Accept  json
// @Produce  json
// @Param data body params.TicketParams true "工单信息"
// @Success 200 {object} apis.Result
// @Router /ticket/update [post]
func TicketUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.TicketParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		// insert a new record
		ticket := service.NewTicketModel(ctx)
		status := ticket.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 处理工单
// @Tags ticket
// @Accept  json
// @Produce  json
// @Param data body params.TicketProcessParams true "表设置信息"
// @Success 200 {object} apis.Result
// @Router /ticket/process [post]
func TicketProcess(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/process", func(c *gin.Context) {
		var param params.TicketProcessParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		// insert a new record
		ticket := service.NewTicketModel(ctx)
		status := ticket.Process(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 删除指定的工单
// @Tags ticket
// @Accept  json
// @Produce  json
// @Param data body params.TicketDeleteParams true "工单Id"
// @Success 200 {object} apis.Result
// @Router /ticket/delete [post]
func TicketDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.TicketDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		dbMod := service.NewTicketModel(ctx)
		status := dbMod.Delete(param)
		ReturnJson(c, status, nil)
	})
}
