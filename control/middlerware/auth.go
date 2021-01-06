package middlerware

import (
	"laser-control/common"
	"laser-control/context"
	// "laser-control/service"

	"github.com/gin-gonic/gin"
)

type authHeader struct {
	Token int `header:"Token"`
}

type Result struct {
	Code    uint32      `json:"Code"`
	Message string      `json:"Message"`
	Data    interface{} `json:"Data"`
}

func AbortJson(c *gin.Context, status *common.Status) {
	c.AbortWithStatusJSON(200, &Result{
		status.Code(),
		status.Error(),
		nil,
	})
}
func Auth(ctx *context.Context) gin.HandlerFunc {
	return func(c *gin.Context) {
		c.Next()
		return
	}
}
