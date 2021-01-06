package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加集群运行信息
// @Tags clusterRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.ClusterRunningMetricsInfo true "集群运行信息"
// @Success 200 {object} apis.Result
// @Router /clusterRunningMetric/insert [post]
func ClusterRunningMetricsInsert(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/insert", func(c *gin.Context) {
		var metricInfo params.ClusterRunningMetricsInfo
		if err := c.ShouldBindJSON(&metricInfo); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewClusterRunningMetricsModel(ctx)
		status := node.Insert(metricInfo)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询集群运行信息
// @Tags clusterRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.ClusterRunningMetricsListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /clusterRunningMetric/list [get]
func ClusterRunningMetricsList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var listParams params.ClusterRunningMetricsListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewClusterRunningMetricsModel(ctx)
		data, total := node.List(listParams)
		ReturnData(c, ListResult{
			Total: total,
			Items: data,
		})
	})
}
