package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加分类
// @Tags machine_category
// @Accept  json
// @Produce  json
// @Param data body params.MachineCategoryParams true "分类信息"
// @Success 200 {object} apis.Result
// @Router /machine_category/store [post]
func MachineCategoryStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.MachineCategoryParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		machineCategory := service.NewMachineCategoryModel(ctx)
		status := machineCategory.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 分类列表
// @Tags machine_category
// @Accept json
// @Produce json
// @Param Page query int true "page offset"
// @Param	Limit query int ture "result number"
// @Success 200	{object} apis.Result
// @Router /machine_category/list	[get]
func MachineCategoryList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.MachineCategoryListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		machineCategory := service.NewMachineCategoryModel(ctx)
		list, total := machineCategory.List(param, true)
		result := make([]params.MachineCategoryDto, 0)
		for _, info := range list {
			machineResult := make([]params.MachineDto, 0)
			for _, machine := range info.Machines {
				machineResult = append(machineResult, params.MachineDto{
					Id:            machine.Id,
					CategoryId:    machine.CategoryId,
					Ip:            machine.Ip,
					CpuCoreNumber: machine.CpuCoreNumber,
					MemorySizeGb:  machine.MemorySizeGb,
					Desc:          machine.Desc,
				})
			}
			result = append(result, params.MachineCategoryDto{
				Id:       info.Id,
				Name:     info.Name,
				Desc:     info.Desc,
				Machines: machineResult,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新分类
// @Tags machine_category
// @Accept  json
// @Produce  json
// @Param data body params.MachineCategoryParams true "分类信息"
// @Success 200 {object} apis.Result
// @Router /machine_category/update [post]
func MachineCategoryUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.MachineCategoryParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		machineCategory := service.NewMachineCategoryModel(ctx)
		status := machineCategory.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 删除分类
// @Tags machine_category
// @Accept  json
// @Produce  json
// @Param data body params.MachineCategoryParams true "分类信息"
// @Success 200 {object} apis.Result
// @Router /machine_category/delete [post]
func MachineCategoryDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.MachineCategoryDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		machineCategory := service.NewMachineCategoryModel(ctx)
		status := machineCategory.Delete(param)
		ReturnJson(c, status, nil)
	})
}
