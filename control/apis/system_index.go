package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个系统指标信息
// @Tags systemIndex
// @Accept  json
// @Produce  json
// @Param data body params.SystemIndexInfo true "系统指标信息"
// @Success 200 {object} apis.Result
// @Router /systemIndex/insert [post]
func SystemIndexInsert(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/insert", func(c *gin.Context) {
		var metricInfo params.SystemIndexInfo
		if err := c.ShouldBindJSON(&metricInfo); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewSystemIndexModel(ctx)
		status := node.Insert(metricInfo)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询系统指标
// @Tags systemIndex
// @Accept  json
// @Produce  json
// @Param data body params.SystemIndexListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /systemIndex/list [get]
func SystemIndexList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var listParams params.SystemIndexListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewSystemIndexModel(ctx)
		data, total := node.List(listParams)
		ReturnData(c, ListResult{
			Total: total,
			Items: data,
		})
	})
}
