package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个集群
// @Tags cluster
// @Accept  json
// @Produce  json
// @Param data body params.ClusterParams true "集群信息"
// @Success 200 {object} apis.Result
// @Router /cluster/store [post]
func ClusterStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.ClusterParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		cluster := service.NewClusterModel(ctx)
		status := cluster.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询集群
// @Tags cluster
// @Accept  json
// @Produce  json
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Success 200 {object} apis.Result
// @Router /cluster/list [get]
func ClusterList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.ClusterListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		cluster := service.NewClusterModel(ctx)
		list, total := cluster.List(param, true)
		result := make([]params.ClusterParams, 0)
		for _, info := range list {
			result = append(result, params.ClusterParams{
				Id:         info.Id,
				Name:       info.Name,
				Alias:      info.Alias,
				ShardTotal: info.ShardTotal,
				Desc:       info.Desc,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新一个集群
// @Tags cluster
// @Accept  json
// @Produce  json
// @Param data body params.ClusterParams true "集群信息"
// @Success 200 {object} apis.Result
// @Router /cluster/update [post]
func ClusterUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.ClusterParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		cluster := service.NewClusterModel(ctx)
		status := cluster.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 同步集群信息
// @Tags cluster
// @Accept  json
// @Produce  json
// @Success 200 {object} apis.Result
// @Router /cluster/synchronize [post]
func ClusterSynchronizeToConsul(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/synchronize", func(c *gin.Context) {
		consul := service.NewConsulModel(ctx)
		status := consul.SynchronizeClusterInfoToConsul()
		ReturnJson(c, status, nil)
	})
}
