package apis

import (
	"github.com/gin-gonic/gin"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"
)

// @Summary 添加一个服务
// @Tags service
// @Accept  json
// @Produce  json
// @Param data body params.ServiceAdd true "服务信息"
// @Success 200 {object} apis.Result
// @Router /service/store [post]
func ServiceStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.ServiceAdd
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		// insert a new record
		service, status := service.NewServiceManager(ctx)
		if status == nil || !status.Ok() {
			ReturnJson(c, status, nil)
			return
		}
		addStatus := service.AddService(param)
		ReturnJson(c, addStatus, nil)
	})
}
