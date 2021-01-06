package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个组
// @Tags group
// @Accept  json
// @Produce  json
// @Param data body params.GroupParams true "组信息"
// @Success 200 {object} apis.Result
// @Router /group/store [post]
func GroupStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.GroupParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		group := service.NewGroupModel(ctx)
		status := group.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询组
// @Tags group
// @Accept  json
// @Produce  json
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Success 200 {object} apis.Result
// @Router /group/list [get]
func GroupList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.GroupListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		group := service.NewGroupModel(ctx)
		list, total := group.List(param, true)
		result := make([]params.GroupDto, 0)
		for _, info := range list {
			activeNum, inactiveNum := uint32(0), uint32(0)
			for _, node := range info.Nodes {
				if *node.Active == service.NodeActive {
					activeNum++
				} else {
					inactiveNum++
				}
			}
			result = append(result, params.GroupDto{
				Id:             info.Id,
				Name:           info.Name,
				Alias:          info.Alias,
				NodeConfigId:   info.NodeConfigId,
				NodeConfigName: info.Config.Name,
				ActiveNumber:   activeNum,
				InactiveNumber: inactiveNum,
				Desc:           info.Desc,
				ClusterId:      info.ClusterId,
				ClusterName:    info.ClusterInfo.Name,
				DcId:           info.DcId,
				DcName:         info.Dc.Name,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新一个组
// @Tags group
// @Accept  json
// @Produce  json
// @Param data body params.GroupParams true "组信息"
// @Success 200 {object} apis.Result
// @Router /group/update [post]
func GroupUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.GroupParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		group := service.NewGroupModel(ctx)
		status := group.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 为某个组设置限流
// @Tags group
// @Accept  json
// @Produce  json
// @Param data body params.GroupReduceMerticsParams true "组限流信息"
// @Success 200 {object} apis.Result
// @Router /group/reduce_metrics [post]
func GroupReduceMetrics(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/reduce_metrics", func(c *gin.Context) {
		var param params.GroupReduceMerticsParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		reduce := service.NewReduceModel(ctx)
		hostShardList := reduce.ReduceMetrics(param)
		result := make([]params.GroupReduceMetricsDto, 0)
		for _, shardList := range hostShardList {
			reduceInfo := make([]params.ReduceMetrics, 0)
			for _, info := range shardList.Metrics {
				reduceInfo = append(reduceInfo, params.ReduceMetrics{
					ShardId:   info.ShardId,
					ReduceNum: info.ReduceNum,
				})
			}
			result = append(result, params.GroupReduceMetricsDto{
				Address:   shardList.Address,
				ShardInfo: reduceInfo,
			})
		}
		ReturnData(c, result)
	})
}

// @Summary 删除指定的组
// @Tags group
// @Accept  json
// @Produce  json
// @Param data body params.GroupDeleteParams true "组Id"
// @Success 200 {object} apis.Result
// @Router /group/delete [post]
func GroupDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.GroupDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		dbMod := service.NewGroupModel(ctx)
		status := dbMod.Delete(param)
		ReturnJson(c, status, nil)
	})
}
