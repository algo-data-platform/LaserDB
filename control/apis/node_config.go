package apis

import (
	"github.com/gin-gonic/gin"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"
)

// @Summary 添加节点配置
// @Tags node_config
// @Accept  json
// @Produce  json
// @Param data body params.NodeConfigParams true "节点配置信息"
// @Success 200 {object} apis.Result
// @Router /node_config/store [post]
func NodeConfigStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.NodeConfigParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		// insert a new record
		node_config := service.NewNodeConfigModel(ctx)
		status := node_config.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询所有节点配置
// @Tags node_config
// @Accept  json
// @Produce  json
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Param eName query string false "配置名"
// @Success 200 {object} apis.Result
// @Router /node_config/list [get]
func NodeConfigList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.NodeConfigListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node_config := service.NewNodeConfigModel(ctx)
		list, total := node_config.List(param, true)
		result := make([]params.NodeConfigDto, 0)
		for _, info := range list {
			rateLimitStrategy := make([]params.NodeRateLimitEntryParams, 0)
			for _, entry := range info.RateLimitStrategy {
				rateLimitEntryParams := params.NodeRateLimitEntryParams{
					Id:              entry.Id,
					BeginHour:       entry.BeginHour,
					EndHour:         entry.EndHour,
					RateBytesPerSec: entry.RateBytesPerSec,
				}
				rateLimitStrategy = append(rateLimitStrategy, rateLimitEntryParams)
			}

			result = append(result, params.NodeConfigDto{
				Id:                  info.Id,
				Name:                info.Name,
				Desc:                info.Desc,
				BlockCacheSizeGb:    info.BlockCacheSizeGb,
				WriteBufferSizeGb:   info.WriteBufferSizeGb,
				NumShardBits:        info.NumShardBits,
				HighPriPoolRatio:    info.HighPriPoolRatio,
				StrictCapacityLimit: info.StrictCapacityLimit,
				RateLimitStrategy:   rateLimitStrategy,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新一个节点表配置
// @Tags node_config
// @Accept  json
// @Produce  json
// @Param data body params.NodeConfigParams true "节点配置信息"
// @Success 200 {object} apis.Result
// @Router /node_config/update [post]
func NodeConfigUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.NodeConfigParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		// insert a new record
		node_config := service.NewNodeConfigModel(ctx)
		status := node_config.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 删除指定的节点配置
// @Tags node_config
// @Accept  json
// @Produce  json
// @Param data body params.NodeConfigDeleteParams true "节点配置Id"
// @Success 200 {object} apis.Result
// @Router /node_config/delete [post]
func NodeConfigDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.NodeConfigDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		dbMod := service.NewNodeConfigModel(ctx)
		status := dbMod.Delete(param)
		ReturnJson(c, status, nil)
	})
}
