package apis

import (
	"github.com/gin-gonic/gin"
	"github.com/hashicorp/consul/api"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"
	// "real.path.com/ads/ads_common_go/service_router"
	// "real.path.com/ads/laser_client"
	"github.com/algo-data-platform/LaserDB/sdk/go/common/laser_client"
	"github.com/algo-data-platform/LaserDB/sdk/go/common/service_router"
)

// @Summary KeyCheck check key info
// @Tags keycheck
// @Accept  json
// @Produce  json
// @Param data body params.KeyCheckParams true       "keycheck info"
// @Success 200 {object} apis.Result
// @Router /keycheck/list [post]
func KeyCheck(router *gin.RouterGroup, ctx *context.Context) {
	service.InitVal(ctx)
	keyCheck := service.NewLaserKeyCheckModel(ctx)
	laserClient := laser_client.NewLaserClient(&laser_client.ClientConfig{
		// consul 配置
		Consul: api.Config{
			Address: ctx.GetConfig().ConsulAddress(),
			Scheme:  ctx.GetConfig().ConsulScheme(),
		},
		// 项目名称
		ProjectName: ctx.GetConfig().ProjectName(),
		// 服务器ip
		LocalIp: ctx.GetConfig().HttpServerHost(),
		// 线上服务 laser_online
		LaserServiceName: service.ServiceName,
	})

	options := laser_client.NewClientOptionFactory(service_router.LoadBalanceMethod_RANDOM,
		ctx.GetConfig().LaserClientTimeout())

	router.POST("/list", func(c *gin.Context) {
		var param params.KeyCheckParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		id, kv := keyCheck.ListValue(param, *laserClient, options)
		var idInfo = make([]params.KeyCheckIdInfo, 0)
		idInfo = append(idInfo, params.KeyCheckIdInfo{
			PartitionId: id.PartitionId,
			ShardId:     id.ShardId,
		})
		ReturnData(c, ListValResult{
			IdInfo: idInfo,
			Items:  kv,
		})
	})
}
