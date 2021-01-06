package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个节点物理资源信息
// @Tags nodePhysicalMetric
// @Accept  json
// @Produce  json
// @Param data body params.NodePhysicalMetricsInfo true "节点物理资源信息"
// @Success 200 {object} apis.Result
// @Router /nodePhysicalMetric/insert [post]
func NodePhysicalMetricsInsert(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/insert", func(c *gin.Context) {
		var metricInfo params.NodePhysicalMetricsInfo
		if err := c.ShouldBindJSON(&metricInfo); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewNodePhysicalMetricsModel(ctx)
		status := node.Insert(metricInfo)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询物理资源信息
// @Tags nodePhysicalMetric
// @Accept  json
// @Produce  json
// @Param data body params.NodePhysicalMetricsListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /nodePhysicalMetric/list [get]
func NodePhysicalMetricsList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var listParams params.NodePhysicalMetricsListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewNodePhysicalMetricsModel(ctx)
		data, total := node.List(listParams)
		ReturnData(c, ListResult{
			Total: total,
			Items: data,
		})
	})
}

// @Summary 查询物理节点列表
// @Tags nodePhysicalMetric
// @Accept  json
// @Produce  json
// @Param data body params.NodeAddressListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /nodePhysicalMetric/node_list [get]
func NodePhysicalMetricsNodeList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/node_list", func(c *gin.Context) {
		var listParams params.NodeAddressListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewNodePhysicalMetricsModel(ctx)
		data := node.ListNodeList(listParams)
		ReturnData(c, data)
	})
}
