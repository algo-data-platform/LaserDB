package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个资源池运行信息
// @Tags resourcePoolRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.ResourcePoolRunningMetricsInfo true "资源池运行信息"
// @Success 200 {object} apis.Result
// @Router /resource_pool_running_metric/insert [post]
func ResourcePoolRunningMetricsInsert(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/insert", func(c *gin.Context) {
		var metricInfo params.ResourcePoolRunningMetricsInfo
		if err := c.ShouldBindJSON(&metricInfo); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewResourcePoolRunningMetricsModel(ctx)
		status := node.Insert(metricInfo)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询资源池运行信息
// @Tags resourcePoolRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.ResourcePoolRunningMetricsListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /resource_pool_running_metric/list [get]
func ResourcePoolRunningMetricsList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var listParams params.ResourcePoolRunningMetricsListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewResourcePoolRunningMetricsModel(ctx)
		data, total := node.List(listParams)
		ReturnData(c, ListResult{
			Total: total,
			Items: data,
		})
	})
}

// @Summary 查询运行群组列表
// @Tags ResourcePoolRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.MetricPoolListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /resource_pool_running_metric/pool_list [get]
func ResourcePoolRunningMetricsPoolList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/pool_list", func(c *gin.Context) {
		var listParams params.MetricPoolListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewResourcePoolRunningMetricsModel(ctx)
		data := node.ListRunningPoolList(listParams)
		ReturnData(c, data)
	})
}
