package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个表运行信息
// @Tags tableMetric
// @Accept  json
// @Produce  json
// @Param data body params.TableMetricsInfo true "表运行信息"
// @Success 200 {object} apis.Result
// @Router /tableMetric/insert [post]
func TableMetricsInsert(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/insert", func(c *gin.Context) {
		var metricInfo params.TableMetricsInfo
		if err := c.ShouldBindJSON(&metricInfo); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewTableMetricsModel(ctx)
		status := node.Insert(metricInfo)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询表运行信息
// @Tags tableMetric
// @Accept  json
// @Produce  json
// @Param data body params.TableMetricsListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /tableMetric/list [get]
func TableMetricsList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var listParams params.TableMetricsListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewTableMetricsModel(ctx)
		data, total := node.List(listParams)
		ReturnData(c, ListResult{
			Total: total,
			Items: data,
		})
	})
}

// @Summary 查询运行群组列表
// @Tags tableMetric
// @Accept  json
// @Produce  json
// @Param data body params.MetricTableListParams true "查询参数"
// @Success 200 {object} apis.Result
// @Router /table_metric/table_list [get]
func TableMetricsTableList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/table_list", func(c *gin.Context) {
		var listParams params.MetricTableListParams
		if err := c.ShouldBindQuery(&listParams); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.NewTableMetricsModel(ctx)
		data := node.ListTableList(listParams)
		ReturnData(c, data)
	})
}
