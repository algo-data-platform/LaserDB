package apis

import (
	"github.com/gin-gonic/gin"
	"laser-control/common"
)

const (
	HttpOkCode = 200
)

type Error struct {
	code    int
	message string
}

type Result struct {
	Code    uint32      `json:"Code"`
	Message string      `json:"Message"`
	Data    interface{} `json:"Data"`
}

type ListResult struct {
	Total uint32      `json:"Total"`
	Items interface{} `json:"Items"`
}

type ListValResult struct {
	IdInfo interface{} `json:"IdInfo"`
	Items  interface{} `json:"Items"`
}

func ReturnJson(c *gin.Context, status *common.Status, data interface{}) {
	c.JSON(HttpOkCode, &Result{
		status.Code(),
		status.Error(),
		data,
	})
}

func ReturnData(c *gin.Context, data interface{}) {
	ReturnJson(c, common.StatusOk(), data)
}

func ReturnOk(c *gin.Context) {
	ReturnJson(c, common.StatusOk(), nil)
}

func ReturnError(c *gin.Context, status *common.Status) {
	ReturnJson(c, status, nil)
}
