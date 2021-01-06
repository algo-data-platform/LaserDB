package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个节点
// @Tags node
// @Accept  json
// @Produce  json
// @Param data body params.NodeParams true "节点信息"
// @Success 200 {object} apis.Result
// @Router /node/store [post]
func NodeStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.NodeParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.GetNodeModel(ctx)
		status := node.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 批量添加节点
// @Tags node
// @Accept  json
// @Produce  json
// @Param data body params.NodeBatchStoreParams true "节点信息"
// @Success 200 {object} apis.Result
// @Router /node/batch_store [post]
func NodeBatchStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/batch_store", func(c *gin.Context) {
		var param params.NodeBatchStoreParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		node := service.GetNodeModel(ctx)
		status := node.BatchStore(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 更新一个节点
// @Tags node
// @Accept  json
// @Produce  json
// @Param data body params.NodeParams true "节点信息"
// @Success 200 {object} apis.Result
// @Router /node/update [post]
func NodeUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.NodeParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		nodeModle := service.GetNodeModel(ctx)
		status := nodeModle.UpdateNode(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 变更节点角色
// @Tags node
// @Accept  json
// @Produce  json
// @Param data body params.NodeChangeRoleParams true "角色变更信息"
// @Success 200 {object} apis.Result
// @Router /node/change_role [post]
func NodeChangeRole(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/change_role", func(c *gin.Context) {
		var param params.NodeChangeRoleParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		nodeModle := service.GetNodeModel(ctx)
		status := nodeModle.ChangeRole(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 删除一个节点
// @Tags node
// @Accept  json
// @Produce  json
// @Param data body params.NodeDeleteParams true "节点删除信息"
// @Success 200 {object} apis.Result
// @Router /node/delete [post]
func NodeDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.NodeDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		nodeModle := service.GetNodeModel(ctx)
		status := nodeModle.DeleteNode(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 批量删除节点
// @Tags node
// @Accept  json
// @Produce  json
// @Param data body params.NodeBatchDeleteParams true "节点删除信息"
// @Success 200 {object} apis.Result
// @Router /node/batch_delete [post]
func NodeBatchDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/batch_delete", func(c *gin.Context) {
		var param params.NodeBatchDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		nodeModle := service.GetNodeModel(ctx)
		status := nodeModle.BatchDeleteNode(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 获取服务节点信息
// @Tags node
// @Accept  json
// @Produce  json
// @Success 200 {object} apis.Result
// @Router /node/list [get]
func NodeList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		nodeManager := service.GetNodeModel(ctx)
		err, groups := nodeManager.ListNode()
		if !err.Ok() {
			ReturnError(c, err)
			return
		}
		ReturnJson(c, common.StatusOk(), groups)
	})
}

// @Summary 获取服务节点信息
// @Tags node
// @Accept  json
// @Produce  json
// @Success 200 {object} apis.Result
// @Router /node/list_shards [get]
func ShardList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list_shards", func(c *gin.Context) {
		shardManager := service.GetShardModelNoSafe()
		shardData := shardManager.GetShards()
		ReturnJson(c, common.StatusOk(), shardData)
	})
}

// @Summary ansible 操作
// @Tags node
// @Accept  json
// @Produce  json
// @Param data body params.NodeBatchOperationParams true "表信息"
// @Success 200 {object} apis.Result
// @Router /node/ansible [post]
func AnsibleOperation(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/ansible", func(c *gin.Context) {
		var param params.NodeBatchOperationParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		manager := service.GetNodeOperatorManager(ctx)
		status, result := manager.AnsibleOperation(param)
		ReturnJson(c, status, result)
	})
}

// @Summary 获取 ansible 操作信息
// @Tags node
// @Accept  json
// @Produce  json
// @Param data body params.NodeBatchOperationInfoParams true "表信息"
// @Success 200 {object} apis.Result
// @Router /node/operation_info [get]
func GetAnsbileOperationInfo(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/operation_info", func(c *gin.Context) {
		var param params.NodeBatchOperationInfoParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		manager := service.GetNodeOperatorManager(ctx)
		status, result := manager.GetOperationInfo(param)
		ReturnJson(c, status, result)
	})
}
