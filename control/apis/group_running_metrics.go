package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个group运行信息
// @Tags groupRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.GroupRunningMetricsInfo true "Group运行信息"
// @Success 200 {object} apis.Result
// @Router /group_running_metric/insert [post]
func GroupRunningMetricsInsert(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/insert", func(c *gin.Context) {
		var metricInfo params.GroupRunningMetricsInfo
		if err := c.ShouldBindJSON(&metricInfo); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewGroupRunningMetricsModel(ctx)
		status := node.Insert(metricInfo)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询group运行信息
// @Tags groupRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.GroupRunningMetricsListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /group_running_metric/list [get]
func GroupRunningMetricsList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var listParams params.GroupRunningMetricsListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewGroupRunningMetricsModel(ctx)
		data, total := node.List(listParams)
		ReturnData(c, ListResult{
			Total: total,
			Items: data,
		})
	})
}

// @Summary 查询运行群组列表
// @Tags groupRunningMetric
// @Accept  json
// @Produce  json
// @Param data body params.MetricGroupListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /group_running_metric/group_list [get]
func GroupRunningMetricsGroupList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/group_list", func(c *gin.Context) {
		var listParams params.MetricGroupListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewGroupRunningMetricsModel(ctx)
		data := node.ListRunningGroupList(listParams)
		ReturnData(c, data)
	})
}
