package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个节点运行信息
// @Tags nodeRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.NodeRunningMetricsInfo true "节点运行信息"
// @Success 200 {object} apis.Result
// @Router /nodeRunningMetric/insert [post]
func NodeRunningMetricsInsert(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/insert", func(c *gin.Context) {
		var metricInfo params.NodeRunningMetricsInfo
		if err := c.ShouldBindJSON(&metricInfo); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewNodeRunningMetricsModel(ctx)
		status := node.Insert(metricInfo)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询节点运行信息
// @Tags nodeRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.NodeRunningMetricsListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /nodeRunningMetric/list [get]
func NodeRunningMetricsList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var listParams params.NodeRunningMetricsListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewNodeRunningMetricsModel(ctx)
		data, total := node.List(listParams)
		ReturnData(c, ListResult{
			Total: total,
			Items: data,
		})
	})
}

// @Summary 查询运行节点列表
// @Tags nodeRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.NodeAddressListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /nodeRunningMetric/node_list [get]
func NodeRunningMetricsNodeList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/node_list", func(c *gin.Context) {
		var listParams params.NodeAddressListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewNodeRunningMetricsModel(ctx)
		data := node.ListRunningNodeList(listParams)
		ReturnData(c, data)
	})
}
