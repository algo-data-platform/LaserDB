package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

func DcStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.DcParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		dcModel := service.NewDcModel(ctx)
		status := dcModel.Store(param)
		ReturnJson(c, status, nil)
	})
}

func DcList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.DcListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		dcModel := service.NewDcModel(ctx)
		list, total := dcModel.List(param, true)
		result := make([]params.DcParams, len(list))
		for idx, _ := range list {
			dc := &list[idx]
			result[idx] = params.DcParams{
				Id:          dc.Id,
				Name:        dc.Name,
				ShardNumber: dc.ShardNumber,
				ClusterId:   dc.ClusterId,
				ClusterName: dc.ClusterInfo.Name,
				Desc:        dc.Desc,
			}
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

func DcUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.DcParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		dcMode := service.NewDcModel(ctx)
		status := dcMode.Update(param)
		ReturnJson(c, status, nil)
	})
}
