package apis

import (
	"github.com/gin-gonic/gin"
	"laser-control/common"
	"laser-control/context"
	"laser-control/service"
)

type Node struct {
	Val int
}

// @Summary 获取用户信息
// @Tags user
// @Accept  json
// @Produce  json
// @Success 200 {object} apis.Result
// @Router /user/info [post]
func UserInfo(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/info", func(c *gin.Context) {

		user := service.NewUserModel(ctx)
		token := c.GetHeader("X-Token")
		service := c.Query("service")
		ctx.Log().Info(service)
		userInfo, err := user.Info(token, service)
		if err != nil && !err.Ok() {
			ReturnError(c, err)
			return
		}

		ReturnJson(c, common.StatusOk(), userInfo)
	})
}
