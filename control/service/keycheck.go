package service

import (
	"laser-control/context"
	"laser-control/params"
	"strconv"

	// "real.path.com/ads/ads_common_go/service_router"
	// "real.path.com/ads/laser_client"
	// "real.path.com/ads/laser_client/if/laser"
	"github.com/algo-data-platform/LaserDB/sdk/go/common/laser_client"
	"github.com/algo-data-platform/LaserDB/sdk/go/laser_client/if/laser"
	"github.com/algo-data-platform/LaserDB/sdk/go/common/service_router"
)

// LaserKeyCheckModel definition
type LaserKeyCheckModel struct {
	ctx *context.Context
}

// NewLaserKeyCheckModel return a instance of KeyCheckModel
func NewLaserKeyCheckModel(ctx *context.Context) *LaserKeyCheckModel {
	instance := &LaserKeyCheckModel{
		ctx: ctx,
	}
	return instance
}

var ServiceName string

func InitVal(ctx *context.Context) {
	ServiceName = "laser_" + ctx.GetConfig().ServiceName()
}

func (laer_key_check_model *LaserKeyCheckModel) getLaserKey(dbName string, tbName string, keyName []string) *laser.LaserKey {
	return &laser.LaserKey{
		DatabaseName: dbName,
		TableName:    tbName,
		PrimaryKeys:  keyName,
		ColumnKeys:   []string{},
	}
}

func (laser_key_check_model *LaserKeyCheckModel) getClientOption(options laser_client.ClientOption, shardId uint32) service_router.ClientOption {
	option := service_router.ClientOption{
		ServiceName:         ServiceName,
		Protocol:            service_router.ServerProtocol_THRIFT,
		ShardId:             int64(shardId),
		Loadbalance:         options.LoadBalance,
		TargetServerAddress: options.TargetServerAddress,
		LocalFirstConfig: service_router.BalanceLocalFirstConfig{
			DiffRange: options.IpDiffRange,
			LocalIp:   laser_key_check_model.ctx.GetConfig().HttpServerHost(),
		},
	}

	switch options.ReadMode {
	case laser_client.LEADER_READ:
		option.ShardType = service_router.ShardType_LEADER
	case laser_client.FOLLOWER_READ:
		option.ShardType = service_router.ShardType_FOLLOWER
	case laser_client.MIXED_READ:
		option.ShardType = service_router.ShardType_ALL
	}

	return option
}

func checkServerLeader(serverCfg service_router.Server, shardId uint32) bool {
	for _, value := range serverCfg.AvailableShardList {
		if shardId == value {
			return true
		}
	}

	return false
}

func (laer_key_check_model *LaserKeyCheckModel) getvalue(
	options laser_client.ClientOption,
	client laser_client.LaserClient,
	serverCfg service_router.Server,
	laserkey laser.LaserKey,
	serverList service_router.ServerList,
	shardId uint32,
) (laser.Status, []params.KeyCheckDto) {
	var status laser.Status
	var val string
	var nodeRole string
	var ret = make([]params.KeyCheckDto, 0)
	if options.ReadMode == laser_client.LEADER_READ {
		status, val = client.GetSync(options, laserkey)
		ret = append(ret, params.KeyCheckDto{
			Role:   "Leader",
			Ip:     serverCfg.Host,
			Port:   serverCfg.Port,
			Dc:     serverCfg.Dc,
			Status: status.String(),
			Value:  val,
		})
	} else {
		for _, server := range serverList {
			options.TargetServerAddress.Host = server.Host
			options.TargetServerAddress.Port = server.Port
			status, val = client.GetSync(options, laserkey)
			check := checkServerLeader(server, shardId)
			if check {
				nodeRole = "Leader"
			} else {
				nodeRole = "Follower"
			}
			ret = append(ret, params.KeyCheckDto{
				Role:   nodeRole,
				Ip:     server.Host,
				Port:   server.Port,
				Dc:     server.Dc,
				Status: status.String(),
				Value:  val,
			})
		}
	}
	return status, ret
}

func (laer_key_check_model *LaserKeyCheckModel) ListValue(param params.KeyCheckParams, client laser_client.LaserClient,
	options laser_client.ClientOption) (params.KeyCheckIdInfo, []params.KeyCheckDto) {
	// 获取shardId
	var idInfo params.KeyCheckIdInfo
	laserkey := laer_key_check_model.getLaserKey(param.Database, param.Table, param.Key)
	shardId, partitionId := client.GetShardIdAndParititionId(*laserkey)
	tableSchema, _ := client.GetTableSchema(laserkey.DatabaseName, laserkey.TableName)
	idInfo.ShardId = strconv.Itoa(int(shardId))
	idInfo.PartitionId = strconv.Itoa(int(partitionId))
	if param.Checkall {
		options.ReadMode = laser_client.MIXED_READ
	} else {
		options.ReadMode = laser_client.LEADER_READ
	}
	// 根据shardId查询ipList
	option := laer_key_check_model.getClientOption(options, uint32(shardId))
	option.Dc = tableSchema.Dc
	router := client.GetServiceRouter()
	serverCfg, ok := router.Discover(option)
	serverList, ok := router.GetServerList(option)
	if !ok {
		var ret = make([]params.KeyCheckDto, 0)
		ret = append(ret, params.KeyCheckDto{
			Role:   "",
			Ip:     "",
			Status: "GetServerList Failed",
			Value:  "",
		})
		idInfo.ShardId = "-"
		idInfo.PartitionId = "-"
		return idInfo, ret
	}
	// 根据IpList，遍历查询每个ip上对应的value值
	status, ret := laer_key_check_model.getvalue(options, client, *serverCfg, *laserkey, serverList, uint32(shardId))
	if status.String() == "SERVICE_NOT_EXISTS_PARTITION" {
		idInfo.ShardId = "-"
		idInfo.PartitionId = "-"
	}
	return idInfo, ret
}
