package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加一个表
// @Tags table
// @Accept  json
// @Produce  json
// @Param data body params.TableAddParams true "表信息"
// @Success 200 {object} apis.Result
// @Router /table/store [post]
func TableStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.TableAddParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		// insert a new record
		table := service.NewTableModel(ctx)
		status := table.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询所有表
// @Tags table
// @Accept  json
// @Produce  json
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Param DatabaseId query int false "DatabaseId"
// @Success 200 {object} apis.Result
// @Router /table/list [get]
func TableList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.TableListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		table := service.NewTableModel(ctx)
		list, total := table.List(param, true)
		result := make([]params.TableDto, 0)
		for _, info := range list {
			result = append(result, params.TableDto{
				Id:              info.Id,
				Name:            info.Name,
				Status:          *info.Status,
				DenyAll:         *info.DenyAll,
				PartitionNumber: info.PartitionNumber,
				Ttl:             *info.Ttl,
				Desc:            info.Desc,
				DcId:            info.DcId,
				DcName:          info.Dc.Name,
				DistDcId:        info.DistDcId,
				DistDcName:      info.DistDc.Name,
				DatabaseId:      info.DatabaseId,
				ConfigId:        info.ConfigId,
				DatabaseName:    info.Database.Name,
				ConfigName:      info.Config.Name,
				EdgeFlowRatio:   info.EdgeFlowRatio,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 查询所有表详细信息
// @Tags table
// @Accept  json
// @Produce  json
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Param DatabaseId query int false "DatabaseId"
// @Param TableId query int false "TableId"
// @Success 200 {object} apis.Result
// @Router /table/list_detail [get]
func TableListDetail(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list_detail", func(c *gin.Context) {
		var param params.TableListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		table := service.NewTableModel(ctx)
		list, total := table.ListDetail(param, true)
		result := make([]params.TableDto, 0)
		for _, info := range list {
			trafficLimits := make([]params.TrafficRestrictionLimitParams, 0)
			for _, limit := range info.TrafficLimits {
				trafficLimit := params.TrafficRestrictionLimitParams{
					Id:            limit.Id,
					Name:          limit.Name,
					OperationType: limit.OperationType,
					LimitType:     limit.LimitType,
					LimitValue:    limit.LimitValue,
				}
				trafficLimits = append(trafficLimits, trafficLimit)
			}

			edgeNodes := make([]params.NodeInfo, 0)
			for _, node := range info.BindEdgeNodes {
				nodeInfo := params.NodeInfo{
					Id:      node.Id,
					NodeId:  node.NodeId,
					GroupId: node.GroupId,
				}
				edgeNodes = append(edgeNodes, nodeInfo)
			}
			result = append(result, params.TableDto{
				Id:              info.Id,
				Name:            info.Name,
				Status:          *info.Status,
				DenyAll:         *info.DenyAll,
				PartitionNumber: info.PartitionNumber,
				Ttl:             *info.Ttl,
				Desc:            info.Desc,
				DcId:            info.DcId,
				DcName:          info.Dc.Name,
				DistDcId:        info.DistDcId,
				DistDcName:      info.DistDc.Name,
				DatabaseId:      info.DatabaseId,
				ConfigId:        info.ConfigId,
				DatabaseName:    info.Database.Name,
				ConfigName:      info.Config.Name,
				EdgeFlowRatio:   info.EdgeFlowRatio,
				TrafficLimits:   trafficLimits,
				BindEdgeNodes:   edgeNodes,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新一个表
// @Tags table
// @Accept  json
// @Produce  json
// @Param data body params.TableUpdateParams true "表信息"
// @Success 200 {object} apis.Result
// @Router /table/update [post]
func TableUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.TableUpdateParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		// insert a new record
		table := service.NewTableModel(ctx)
		status := table.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 同步表信息
// @Tags table
// @Accept  json
// @Produce  json
// @Success 200 {object} apis.Result
// @Router /table/synchronize [post]
func TableSynchronizeToConsul(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/synchronize", func(c *gin.Context) {
		consul := service.NewConsulModel(ctx)
		status := consul.SynchronizeDatabaseTableSchemaToConsul()
		ReturnJson(c, status, nil)
	})
}

// @Summary 列出所有 laser 支持的命令
// @Tags table
// @Accept  json
// @Produce  json
// @Success 200 {object} apis.Result
// @Router /table/list_commands [get]
func TableListAllCommands(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list_commands", func(c *gin.Context) {
		commands := make([]params.LaserCommandParams, 0)
		singleOperations := ctx.GetConfig().SingleOperations()
		for _, command := range singleOperations {
			commandParam := params.LaserCommandParams{
				Name:          command,
				OperationType: service.SingleOperation,
			}
			commands = append(commands, commandParam)
		}
		multipleOperations := ctx.GetConfig().MultipleOperations()
		for _, command := range multipleOperations {
			commandParam := params.LaserCommandParams{
				Name:          command,
				OperationType: service.MultipleOperation,
			}
			commands = append(commands, commandParam)
		}

		ReturnData(c, ListResult{
			Total: uint32(len(commands)),
			Items: commands,
		})
	})
}

// @Summary 删除指定的表
// @Tags table
// @Accept  json
// @Produce  json
// @Param data body params.TableDeleteParams true "表Id"
// @Success 200 {object} apis.Result
// @Router /table/delete [post]
func TableDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.TableDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		table := service.NewTableModel(ctx)
		status := table.Delete(param)
		ReturnJson(c, status, nil)
	})
}
