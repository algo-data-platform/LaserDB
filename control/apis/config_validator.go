package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 校验 group 是否可以作为主副本
// @Tags config_validator
// @Accept  json
// @Produce  json
// @Param GroupName query string true "组名称"
// @Success 200 {object} apis.Result
// @Router /config_validator/check_group_as_master [get]
func CheckGroupReadyToBeMaster(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/check_group_as_master", func(c *gin.Context) {
		var param params.CheckGroupReadyToBeMasterParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		validator := service.NewConfigValidator(ctx)
		result, status := validator.CheckGroupReadyToBeMaster(param)
		ReturnJson(c, status, result)
	})
}

// @Summary 校验 本地配置和 Consul 配置是否一致
// @Tags config_validator
// @Accept  json
// @Produce  json
// @Success 200 {object} apis.Result
// @Router /config_validator/unsynchronized_config [get]
func CheckUnsynchronizedConfig(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/unsynchronized_config", func(c *gin.Context) {
		validator := service.NewConfigValidator(ctx)
		data := validator.CheckUnsynchronizedConfig()
		ReturnData(c, data)
	})
}
