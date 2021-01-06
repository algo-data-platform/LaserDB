package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 获取表对应的 shard list
// @Tags tools
// @Accept  json
// @Produce  json
// @Param DatabaseName query string true "DatabaseName"
// @Param TableName query string true "TableName"
// @Param Type query int true "Type"
// @Param AssignedListNum query int true "AssignedListNum"
// @Success 200 {object} apis.Result
// @Router /tools/assign_table_shard_list [get]
func AssignTableShardList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/assign_table_shard_list", func(c *gin.Context) {
		var param params.AssignShardListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		tableShardModel := service.NewTableShardAssignerModel(ctx)
		shardLists, status := tableShardModel.AssignTableShardList(param)
		ReturnJson(c, status, shardLists)
	})
}
