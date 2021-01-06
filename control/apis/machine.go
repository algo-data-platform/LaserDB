package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加机器资源
// @Tags machine
// @Accept  json
// @Produce  json
// @Param data body params.MachineParams true "机器资源信息"
// @Success 200 {object} apis.Result
// @Router /machine/store [post]
func MachineStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.MachineParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		machine := service.NewMachineModel(ctx)
		status := machine.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 机器资源列表
// @Tags machine
// @Accept json
// @Produce json
// @Param Page query int true "page offset"
// @Param	Limit query int ture "result number"
// @Success 200	{object} apis.Result
// @Router /machine/list	[get]
func MachineList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.MachineListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		machine := service.NewMachineModel(ctx)
		list, total := machine.List(param, true)
		result := make([]params.MachineDto, 0)
		for _, info := range list {
			result = append(result, params.MachineDto{
				Id:                  info.Id,
				CategoryId:          info.CategoryId,
				Ip:                  info.Ip,
				CpuCoreNumber:       info.CpuCoreNumber,
				MemorySizeGb:        info.MemorySizeGb,
				Desc:                info.Desc,
				MachineCategoryName: info.MachineCategory.Name,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新机器资源
// @Tags machine
// @Accept  json
// @Produce  json
// @Param data body params.MachineUpdateParams true "机器资源信息"
// @Success 200 {object} apis.Result
// @Router /machine/update [post]
func MachineUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.MachineUpdateParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		machineCategory := service.NewMachineModel(ctx)
		status := machineCategory.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 删除机器资源
// @Tags machine
// @Accept  json
// @Produce  json
// @Param data body params.MachineDeleteParams true "机器资源ID"
// @Success 200 {object} apis.Result
// @Router /machine/delete [post]
func MachineDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.MachineDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		machineCategory := service.NewMachineModel(ctx)
		status := machineCategory.Delete(param)
		ReturnJson(c, status, nil)
	})
}
