package apis

import (
	"github.com/gin-gonic/gin"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"
)

// @Summary DBStore add new database info
// @Tags database
// @Accept json
// @Produce json
// @Param data body params.DatabaseAddParams true	"database info"
// @Success 200	{object} apis.Result
// @Router /database/store	[post]
func DBStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.DatabaseAddParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		dbMod := service.NewLaserDatabaseModel(ctx)
		status := dbMod.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary DBList query table info
// @Tags database
// @Accept json
// @Produce json
// @Param Page query int true "page offset"
// @Param	Limit query int ture "result number"
// @Success 200	{object} apis.Result
// @Router /database/list	[get]
func DBList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.DatabaseListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		dbMod := service.NewLaserDatabaseModel(ctx)
		list, total := dbMod.List(param, true)
		result := make([]params.DatabaseDto, 0)
		for _, info := range list {
			result = append(result, params.DatabaseDto{
				Id:   info.Id,
				Name: info.Name,
				Desc: info.Desc,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary DBUpdate update database info
// @Tags database
// @Accept json
// @Produce json
// @Param data body params.DatabaseUpdateParams true "database info"
// @Success 200	{object} apis.Result
// @Router /database/update	[post]
func DBUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.DatabaseUpdateParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		dbMod := service.NewLaserDatabaseModel(ctx)
		status := dbMod.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 删除指定的库
// @Tags database
// @Accept  json
// @Produce  json
// @Param data body params.DatabaseDeleteParams true "数据库Id"
// @Success 200 {object} apis.Result
// @Router /database/delete [post]
func DBDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.DatabaseDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		dbMod := service.NewLaserDatabaseModel(ctx)
		status := dbMod.Delete(param)
		ReturnJson(c, status, nil)
	})
}
