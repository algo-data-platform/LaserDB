package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"
	"github.com/gin-gonic/gin"
)

// @Summary 为某张表添加具体的timeout以及限流配置
// @Tags proxy_config
// @Accept  json
// @Produce  json
// @Param data body params.ProxyTableConfigParams true "表配置信息"
// @Success 200 {object} apis.Result
// @Router /proxy_config/store [post]
func ProxyTableConfigStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.ProxyTableConfigParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		// insert a new record
		proxy_table_config := service.NewProxyTableConfigModel(ctx)
		status := proxy_table_config.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询所有表timeout以及限流配置
// @Tags proxy_config
// @Accept  json
// @Produce  json
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Param DatabaseName query string false "数据库名"
// @Success 200 {object} apis.Result
// @Router /proxy_config/list [get]
func ProxyTableConfigList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.ProxyTableConfigListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		proxy_table_config := service.NewProxyTableConfigModel(ctx)
		list, total := proxy_table_config.List(param, true)
		result := make([]params.ProxyTableConfigDto, 0)
		for _, info := range list {
			result = append(result, params.ProxyTableConfigDto{
				Id:           info.Id,
				DatabaseName: info.DatabaseName,
				TableName:    info.TableName,
				ReadTimeout:  info.ReadTimeout,
				WriteTimeout: info.WriteTimeout,
				AllowedFlow:  info.AllowedFlow,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新一个表timeout以及限流配置
// @Tags proxy_config
// @Accept  json
// @Produce  json
// @Param data body params.ProxyTableConfigParams true "表配置信息"
// @Success 200 {object} apis.Result
// @Router /proxy_config/update [post]
func ProxyTableConfigUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.ProxyTableConfigParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		// insert a new record
		proxy_table_config := service.NewProxyTableConfigModel(ctx)
		status := proxy_table_config.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 同步表timeout以及限流配置
// @Tags proxy_config
// @Accept  json
// @Produce  json
// @Success 200 {object} apis.Result
// @Router /proxy_config/synchronize [post]
func ProxyTableConfigSynchronizeToConsul(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/synchronize", func(c *gin.Context) {
		consul := service.NewConsulModel(ctx)
		status := consul.SynchronizeProxyTableConfigSchemaToConsul()
		ReturnJson(c, status, nil)
	})
}
