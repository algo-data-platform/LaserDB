package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个表配置
// @Tags table_config
// @Accept  json
// @Produce  json
// @Param data body params.TableConfigParams true "表配置信息"
// @Success 200 {object} apis.Result
// @Router /table_config/store [post]
func TableConfigStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.TableConfigParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		// insert a new record
		tableConfig := service.NewTableConfigModel(ctx)
		status := tableConfig.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询表配置
// @Tags table_config
// @Accept  json
// @Produce  json
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Param Name query string false "配置名称"
// @Param ExcludedNames query []string false "排除配置名称"
// @Success 200 {object} apis.Result
// @Router /table_config/list [get]
func TableConfigList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.TableConfigListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		tableConfig := service.NewTableConfigModel(ctx)
		list, total := tableConfig.List(param, true)
		result := make([]params.TableConfigParams, 0)
		for _, info := range list {
			result = append(result, params.TableConfigParams{
				Id:        info.Id,
				Name:      info.Name,
				Version:   info.Version,
				IsDefault: info.IsDefault,
				Desc:      info.Desc,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 查询表配置（带有具体的配置项）
// @Tags table_config
// @Accept  json
// @Produce  json
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Param Name query string false "配置名称"
// @Param ExcludedNames query []string false "排除配置名称"
// @Success 200 {object} apis.Result
// @Router /table_config/list_detail [get]
func TableConfigListWithDetailItems(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list_detail", func(c *gin.Context) {
		var param params.TableConfigListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		tableConfig := service.NewTableConfigModel(ctx)
		list, total := tableConfig.ListWithDetailItems(param, true)
		result := make([]params.TableConfigParams, 0)
		for _, info := range list {
			configItems := make([]params.TableConfigItemParams, 0)
			for _, item := range info.ConfigItems {
				configItem := params.TableConfigItemParams{
					Id:    item.Id,
					Name:  item.Name,
					Value: item.Value,
					Type:  item.Type,
				}
				configItems = append(configItems, configItem)
			}
			result = append(result, params.TableConfigParams{
				Id:          info.Id,
				Name:        info.Name,
				Version:     info.Version,
				IsDefault:   info.IsDefault,
				Desc:        info.Desc,
				ConfigItems: configItems,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新一个表配置
// @Tags table_config
// @Accept  json
// @Produce  json
// @Param data body params.TableConfigParams true "表信息"
// @Success 200 {object} apis.Result
// @Router /table_config/update [post]
func TableConfigUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.TableConfigParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		tableConfig := service.NewTableConfigModel(ctx)
		status := tableConfig.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 删除指定的数据表配置
// @Tags table_config
// @Accept  json
// @Produce  json
// @Param data body params.TableConfigDeleteParams true "数据表配置Id"
// @Success 200 {object} apis.Result
// @Router /table_config/delete [post]
func TableConfigDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.TableConfigDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		tableConfig := service.NewTableConfigModel(ctx)
		status := tableConfig.Delete(param)
		ReturnJson(c, status, nil)
	})
}
