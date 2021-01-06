package laser_client

import (
	"context"
	"errors"
	"math/rand"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/algo-data-platform/LaserDB/sdk/go/common/ip"
	"github.com/algo-data-platform/LaserDB/sdk/go/common/service_router"
	"github.com/algo-data-platform/LaserDB/sdk/go/laser_client/if/laser"
	"github.com/algo-data-platform/LaserDB/sdk/go/laser_client/lib"
	"github.com/facebook/fbthrift/thrift/lib/go/thrift"
	"github.com/hashicorp/consul/api"
	"github.com/liubang/tally"
)

type ReadMode int

const (
	LEADER_READ ReadMode = iota
	FOLLOWER_READ
	MIXED_READ
)

const (
	defaultReceiveTimeoutMs = 20
	projectPrefix           = "laser_go_"
)

var ErrTimeout = errors.New("request timeout")

type ClientOption struct {
	MaxConnPerServer        int                                    `json:"MaxConnPerServer" toml:"MaxConnPerServer"`
	ConnIdleTimeoutMs       int                                    `json:"ConnIdleTimeoutMs" toml:"ConnIdleTimeoutMs"`
	ReceiveTimeoutMs        int                                    `json:"ReceiveTimeoutMs" toml:"ReceiveTimeoutMs"`
	ConnectionRetry         int                                    `json:"ConnectionRetry" toml:"ConnectionRetry"`
	TimeoutRetry            int                                    `json:"TimeoutRetry" toml:"TimeoutRetry"`
	ThriftTransport         service_router.ThriftTransportType     `json:"ThriftTransport" toml:"ThriftTransport"`
	ThriftCompressionMethod service_router.ThriftCompressionMethod `json:"ThriftCompressionMethod" toml:"ThriftCompressionMethod"`
	LoadBalance             string                                 `json:"LoadBalance" toml:"LoadBalance"`
	IpDiffRange             int                                    `json:"IpDiffRange" toml:"IpDiffRange"`
	ReadMode                ReadMode                               `json:"ReadMode" toml:"ReadMode"`
	TargetServerAddress     service_router.ServerAddress           `json:"TargetServerAddress" toml:"TargetServerAddress"`
}

func (co ClientOption) Validate() error {
	if co.MaxConnPerServer < 0 {
		return errors.New("option MaxConnPerServer can not be negative")
	}
	if co.ConnIdleTimeoutMs < 0 {
		return errors.New("option ConnIdleTimeoutMs can not be negative")
	}
	if co.ReceiveTimeoutMs < 0 {
		return errors.New("option.ReceiveTimeoutMs can not be negative")
	}
	if co.TimeoutRetry < 0 {
		return errors.New("option TimeoutRetry can not be negative")
	}
	if lb, ok := service_router.SupportedLoadBalanceMethod[co.LoadBalance]; !ok || !lb {
		return errors.New("unsupported loadbalance method")
	}
	return nil
}

type ClientConfig struct {
	Consul           api.Config
	ProjectName      string
	LocalIp          string
	HttpPort         int
	LaserServiceName string
	Multiplexer      service_router.Multiplexer
}

func NewClientOptionFactory(loadBalance string, timeoutMs int) ClientOption {
	return ClientOption{
		MaxConnPerServer:        runtime.NumCPU(),
		ConnIdleTimeoutMs:       0,
		ConnectionRetry:         0,
		TimeoutRetry:            0,
		ReceiveTimeoutMs:        timeoutMs,
		ThriftTransport:         service_router.THRIFT_TRANSPORT_HEADER,
		ThriftCompressionMethod: service_router.ThriftCompressionMethod_None,
		ReadMode:                FOLLOWER_READ,
		LoadBalance:             loadBalance,
		IpDiffRange:             65536,
	}
}

type ThriftProcessRequestFunc func(c *laser.LaserServiceClient) (*laser.LaserResponse, error)

type LaserClient struct {
	laserServiceName string
	localIp          string
	router           *service_router.Router
	cg               *service_router.ConnGroup
	configManager    *lib.ConfigManager
	meters           map[string]tally.Meter
	timers           map[string]tally.Timer
	poolMetrics      map[string]tally.Scope
	mutex            sync.RWMutex
}

func (client *LaserClient) getPoolMetric(host string, port uint16) tally.Scope {
	key := host + ":" + strconv.Itoa(int(port))
	client.mutex.RLock()
	metric, ok := client.poolMetrics[key]
	client.mutex.RUnlock()
	if !ok {
		client.mutex.Lock()
		metric, ok = client.poolMetrics[key]
		if !ok {
			metric = client.router.GetMetrics().Tagged(map[string]string{"addr": key})
			client.poolMetrics[key] = metric
		}
		client.mutex.Unlock()
	}
	return metric
}

type callResp struct {
	idxes  []int
	status laser.Status
	resp   *laser.LaserResponse
}

func (client *LaserClient) parseException(exception error) (laser.Status, bool) {
	var (
		laserExceptionType *laser.LaserException
		status             laser.Status
		ok                 bool
	)
	if errors.As(exception, &laserExceptionType) {
		status = exception.(*laser.LaserException).GetStatus()
		ok = true
	} else {
		client.meters[LASER_CLIENT_THRIFT_GET_STATUS_ERROR].Mark(1)
		// default
		status = laser.Status_CLIENT_THRIFT_CALL_ERROR
		ok = false
	}
	if status == laser.Status_RS_KEY_EXPIRE {
		client.meters[LASER_CLIENT_THRIFT_CALL_RS_KEY_EXPIRE].Mark(1)
	} else if status == laser.Status_RS_NOT_FOUND {
		client.meters[LASER_CLIENT_THRIFT_CALL_RS_NOT_FOUND].Mark(1)
	} else {
		client.meters[LASER_CLIENT_THRIFT_CALL_ERROR].Mark(1)
	}
	return status, ok
}

func newLaserServiceClient(transport thrift.Transport, protocol thrift.ProtocolFactory) service_router.ThriftClient {
	return laser.NewLaserServiceClientFactory(transport, protocol)
}

func closeThriftClient(conn service_router.ThriftClient) error {
	return conn.(*laser.LaserServiceClient).Transport.Close()
}

func thriftIsOpen(conn service_router.ThriftClient) bool {
	return conn.(*laser.LaserServiceClient).Transport.IsOpen()
}

func checkTargetServerAddress(address *service_router.ServerAddress) bool {
	if nil != address && len(address.Host) > 0 && address.Port > 0 {
		return true
	}
	return false
}

func (client *LaserClient) commonCall(
	laserKey laser.LaserKey,
	options ClientOption,
	callFunc ThriftProcessRequestFunc,
) *callResp {
	shardId, partitionHash, routeToEdgeNode, dc := client.getRouteInfo(laserKey, options.ReadMode)
	return client.commonCallWithRouteId(shardId, partitionHash, routeToEdgeNode, dc, options, callFunc)
}

func (client *LaserClient) commonCallWithRouteId(
	shardId int64,
	partitionHash int64,
	routeToEdgeNode bool,
	dc string,
	options ClientOption,
	callFunc ThriftProcessRequestFunc,
) *callResp {
	callTimes := 0
	routerOption := client.getClientOption(&options, shardId, partitionHash, routeToEdgeNode, dc)
	if options.ReceiveTimeoutMs <= 0 {
		options.ReceiveTimeoutMs = defaultReceiveTimeoutMs
	}
RETRY:
	var (
		server *service_router.Server
		ok     bool
	)
	callTimes++
	if checkTargetServerAddress(&options.TargetServerAddress) {
		server = &service_router.Server{
			Host: options.TargetServerAddress.Host,
			Port: options.TargetServerAddress.Port,
		}
		client.router.GetCounter(server).Inc(1)
	} else {
		connectTimes := 0
	GETCONN:
		connectTimes++
		discoverTimer := client.timers[LASER_CLIENT_SERVICE_ROUTER_DISCOVER_TIMER].Start()
		server, ok = client.router.Discover(*routerOption)
		discoverTimer.Stop()
		if !ok {
			if options.ConnectionRetry >= connectTimes {
				goto GETCONN
			}
			return &callResp{status: laser.Status_CLIENT_THRIFT_CALL_ERROR, resp: &laser.LaserResponse{}}
		}
	}
	thriftConfig := service_router.ThriftConfig{
		Host:              server.Host,
		Port:              int(server.Port),
		TransportType:     options.ThriftTransport,
		CompressionMethod: options.ThriftCompressionMethod,
		NewThriftClient:   newLaserServiceClient,
		CloseThriftClient: closeThriftClient,
		ThriftIsOpen:      thriftIsOpen,
	}
	connTimer := client.timers[LASER_CLIENT_GET_CONNECTION_TIMER].Start()
	thriftConn, err := client.cg.GetConnection(
		thriftConfig,
		service_router.PoolMaxActive(options.MaxConnPerServer),
		service_router.PoolWait(false),
		service_router.PoolIdleTimeout(time.Duration(options.ConnIdleTimeoutMs)*time.Millisecond),
		service_router.PoolMetric(client.getPoolMetric(server.Host, server.Port)))
	connTimer.Stop()
	if err != nil {
		client.meters[LASER_CLIENT_GET_CONNECTION_ERROR].Mark(1)
		return &callResp{status: laser.Status_CLIENT_THRIFT_CALL_ERROR, resp: &laser.LaserResponse{}}
	}

	client.meters[LASER_CLIENT_METRIC_CALL_SERVER_TIMES].Mark(1)
	resp := func() <-chan *callResp {
		ch := make(chan *callResp, 1)
		go func() {
			thriftConn.Do(func(conn interface{}) (interface{}, error) {
				thriftCallTimer := client.timers[LASER_CLIENT_THRIFT_CALL_TIMER].Start()
				callResult, err := callFunc(conn.(*laser.LaserServiceClient))
				thriftCallTimer.Stop()
				return callResult, err
			}).Done(func() {
				//thriftConn.Close()
			}).OnError(func(err error) {
				status, _ := client.parseException(err)
				thriftConn.SetErr(err)
				thriftConn.Close()
				ch <- &callResp{status: status, resp: &laser.LaserResponse{}}
			}).OnSuccess(func(resp interface{}) {
				thriftConn.Close()
				ch <- &callResp{status: laser.Status_OK, resp: resp.(*laser.LaserResponse)}
			})
		}()
		return ch
	}()

	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(options.ReceiveTimeoutMs)*time.Millisecond)
	defer cancel()
	select {
	case <-ctx.Done():
		client.meters[LASER_CLIENT_THRIFT_CALL_TIMEOUT].Mark(1)
		thriftConn.SetErr(ErrTimeout)
		if options.TimeoutRetry >= callTimes {
			goto RETRY
		}
		return &callResp{status: laser.Status_CLIENT_THRIFT_CALL_TIMEOUT, resp: &laser.LaserResponse{}}
	case res := <-resp:
		return res
	}
}

func (client *LaserClient) getClientOption(options *ClientOption, shardId int64, partitionHash int64, routeToEdgeNode bool, dc string) *service_router.ClientOption {
	option := &service_router.ClientOption{
		ServiceName:         client.laserServiceName,
		Protocol:            service_router.ServerProtocol_THRIFT,
		ShardId:             shardId,
		PartitionHash:       partitionHash,
		RouteToEdgeNode:     routeToEdgeNode,
		Loadbalance:         options.LoadBalance,
		TargetServerAddress: options.TargetServerAddress,
		Dc:                  dc,
		LocalFirstConfig: service_router.BalanceLocalFirstConfig{
			DiffRange: options.IpDiffRange,
			LocalIp:   client.localIp,
		},
	}

	switch options.ReadMode {
	case LEADER_READ:
		option.ShardType = service_router.ShardType_LEADER
	case FOLLOWER_READ:
		option.ShardType = service_router.ShardType_FOLLOWER
	case MIXED_READ:
		option.ShardType = service_router.ShardType_ALL
	}

	return option
}

func (client *LaserClient) GetTableSchema(databaseName, tableName string) (lib.TableSchema, bool) {
	return client.configManager.GetTableSchema(databaseName, tableName)
}

func (client *LaserClient) GetShardIdAndParititionId(key laser.LaserKey) (int64, uint32) {
	databaseName := key.GetDatabaseName()
	tableName := key.GetTableName()
	primaryKeys := key.GetPrimaryKeys()
	columnKeys := key.GetColumnKeys()
	tableSchema, ok := client.configManager.GetTableSchema(databaseName, tableName)
	if !ok {
		return 0, 0
	}
	formatKey := lib.NewLaserKeyFormat(primaryKeys, columnKeys)
	partitionId := lib.GetPartitionId(databaseName, tableName, formatKey, tableSchema.PartitionNumber)
	partition := lib.NewPartition(databaseName, tableName, partitionId)
	return lib.GetShardId(partition, client.configManager, tableSchema.Dc), partitionId
}

func (client *LaserClient) getRouteInfo(key laser.LaserKey, readMode ReadMode) (int64, int64, bool, string) {
	databaseName := key.GetDatabaseName()
	tableName := key.GetTableName()
	primaryKeys := key.GetPrimaryKeys()
	columnKeys := key.GetColumnKeys()
	tableSchema, ok := client.configManager.GetTableSchema(databaseName, tableName)
	if !ok {
		return 0, 0, false, service_router.DEFAULT_DC
	}
	formatKey := lib.NewLaserKeyFormat(primaryKeys, columnKeys)
	partitionId := lib.GetPartitionId(databaseName, tableName, formatKey, tableSchema.PartitionNumber)
	partition := lib.NewPartition(databaseName, tableName, partitionId)

	shardId := lib.GetShardId(partition, client.configManager, tableSchema.Dc)
	partitionHash := partition.GetPartitionHash()
	routeToEdgeNode := false
	if nil != tableSchema.BindEdgeNodes && readMode != LEADER_READ {
		randomValue := rand.Intn(100) //返回[0,100)的随机整数
		if randomValue < tableSchema.EdgeFlowRatio {
			routeToEdgeNode = true
		}
	}
	return shardId, partitionHash, routeToEdgeNode, tableSchema.Dc
}

func (client *LaserClient) init() {
	client.cg = service_router.GetConnGroup()
	client.configManager = lib.NewConfigManager(client.laserServiceName, client.router)
	metric := client.router.GetMetrics()
	tag := map[string]string{"module": LASER_CLIENT_MODULE_NAME}
	// version
	metric.Tagged(tag).Tagged(map[string]string{"type": LASER_CLIENT_MAJOR_VERSION}).Gauge(LASER_CLIENT_VERSION).Update(MAJOR_VERSION)
	metric.Tagged(tag).Tagged(map[string]string{"type": LASER_CLIENT_MINOR_VERSION}).Gauge(LASER_CLIENT_VERSION).Update(MINOR_VERSION)
	metric.Tagged(tag).Tagged(map[string]string{"type": LASER_CLIENT_REVISION}).Gauge(LASER_CLIENT_VERSION).Update(REVISION)

	client.meters[LASER_CLIENT_METRIC_CALL_SERVER_TIMES] = metric.Tagged(tag).Meter(LASER_CLIENT_METRIC_CALL_SERVER_TIMES)
	client.meters[LASER_CLIENT_THRIFT_CALL_RS_NOT_FOUND] = metric.Tagged(tag).Meter(LASER_CLIENT_THRIFT_CALL_RS_NOT_FOUND)
	client.meters[LASER_CLIENT_THRIFT_CALL_RS_KEY_EXPIRE] = metric.Tagged(tag).Meter(LASER_CLIENT_THRIFT_CALL_RS_KEY_EXPIRE)
	client.meters[LASER_CLIENT_THRIFT_CALL_TIMEOUT] = metric.Tagged(tag).Meter(LASER_CLIENT_THRIFT_CALL_TIMEOUT)
	client.meters[LASER_CLIENT_THRIFT_CALL_ERROR] = metric.Tagged(tag).Meter(LASER_CLIENT_THRIFT_CALL_ERROR)
	client.meters[LASER_CLIENT_THRIFT_GET_STATUS_ERROR] = metric.Tagged(tag).Meter(LASER_CLIENT_THRIFT_GET_STATUS_ERROR)
	client.meters[LASER_CLIENT_GET_CONNECTION_ERROR] = metric.Tagged(tag).Meter(LASER_CLIENT_GET_CONNECTION_ERROR)

	client.timers[LASER_CLIENT_THRIFT_CALL_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_THRIFT_CALL_TIMER)
	client.timers[LASER_CLIENT_GET_CONNECTION_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_GET_CONNECTION_TIMER)
	client.timers[LASER_CLIENT_SERVICE_ROUTER_DISCOVER_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_SERVICE_ROUTER_DISCOVER_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_DEL_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_DEL_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_EXPIRE_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_EXPIRE_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_EXPIREAT_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_EXPIREAT_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_TTL_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_TTL_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_GET_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_GET_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_SET_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_SET_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_APPEND_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_APPEND_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_SETX_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_SETX_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_MSET_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_MSET_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_MSET_DETAIL_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_MSET_DETAIL_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_MGET_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_MGET_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_MGET_DETAIL_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_MGET_DETAIL_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_EXIST_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_EXIST_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_HSET_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_HSET_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_HGET_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_HGET_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_HDEL_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_HDEL_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_HEXISTS_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_HEXISTS_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_HGETALL_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_HGETALL_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_HKEYS_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_HKEYS_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_HLEN_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_HLEN_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_HMSET_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_HMSET_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_HMGET_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_HMGET_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_LLEN_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_LLEN_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_LPOP_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_LPOP_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_LPUSH_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_LPUSH_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_RPOP_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_RPOP_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_RPUSH_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_RPUSH_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_LRANGE_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_LRANGE_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_LINDEX_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_LINDEX_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_DECR_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_DECR_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_INCR_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_INCR_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_DECRBY_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_DECRBY_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_INCRBY_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_INCRBY_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_ZADD_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_ZADD_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_ZRANGEBYSCORE_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_ZRANGEBYSCORE_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_SADD_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_SADD_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_SISMEMBER_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_SISMEMBER_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_SREMOVE_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_SREMOVE_TIMER)
	client.timers[LASER_CLIENT_METRIC_COMMAND_SMEMBERS_TIMER] = metric.Tagged(tag).Timer(LASER_CLIENT_METRIC_COMMAND_SMEMBERS_TIMER)

}

func (client *LaserClient) GetSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_GET_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Get(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetStringData()
}

func (client *LaserClient) SetSync(clientOption ClientOption, kv laser.LaserKV) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_SET_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(*kv.GetKey(), clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Sset(&kv)
	})
	st.Stop()
	return callResp.status
}

func (client *LaserClient) AppendSync(clientOption ClientOption, key laser.LaserKey, val string) (laser.Status, int64) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_APPEND_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Append(&key, val)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetIntData()
}

func (client *LaserClient) SetxSync(clientOption ClientOption, kv laser.LaserKV, setOption laser.LaserSetOption) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_SETX_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(*kv.GetKey(), clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Setx(&kv, &setOption)
	})
	st.Stop()
	return callResp.status
}

func (client *LaserClient) ExpireSync(clientOption ClientOption, key laser.LaserKey, time int64) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_EXPIRE_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Expire(&key, time)
	})
	st.Stop()
	return callResp.status
}

func (client *LaserClient) ExpireAtSync(clientOption ClientOption, key laser.LaserKey, time int64) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_EXPIREAT_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.ExpireAt(&key, time)
	})
	st.Stop()
	return callResp.status
}

func (client *LaserClient) DelSync(clientOption ClientOption, key laser.LaserKey) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_DEL_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Delkey(&key)
	})
	st.Stop()
	return callResp.status
}

type keyToIndex struct {
	kv  *laser.LaserKV
	idx int
}

type addressKeyMap struct {
	address        *service_router.ServerAddress
	keyToIndexList []*keyToIndex
}

type dcGroupInfo struct {
	routeId2Indexs         map[int64][]*keyToIndex
	uniqueShardIds         []int64
	uniquePartitionHashes  []int64
	partitionHash2ShardIds map[int64]int64
	shardId                int64
	partitionHash          int64
	routeToEdgeNode        bool
}

func newDcGroupInfo() *dcGroupInfo {
	return &dcGroupInfo{
		routeId2Indexs:         make(map[int64][]*keyToIndex),
		uniqueShardIds:         make([]int64, 0),
		uniquePartitionHashes:  make([]int64, 0),
		partitionHash2ShardIds: make(map[int64]int64),
	}
}

const NIL_ADDRESS = "nil_address"

func (client *LaserClient) group(kvs []laser.LaserKV, clientOption *ClientOption) map[string]*addressKeyMap {
	dcGroupInfos := make(map[string]*dcGroupInfo)
	result := make(map[string]*addressKeyMap)

	// 1. 一个请求使用 route_id 作为路由到目的 Server 的依据，route_id 可以是 shard_id, 也可以是 partition_hash :
	//    	被分配到主集群的请求用 shard_id 作为 route_id;
	//    	被分配到边缘节点的请求用 partition_hash 作为 route_id;
	// 2. 所以我们把所有的请求分成两部分 :
	// 		一部分在主集群中进行服务发现，这部分请求的 route_id(shard_id)去重放到uniqueShardIds 中;
	//    	另一部分在边缘节点中进行服务发现，这部分请求的 route_id(partition_hash)去重放到 uniquePartitionHashes 中;
	// 3. 服务发现的时候 :
	// 		使用 uniqueShardIds 作为 batchDiscover 参数去主集群进行服务发现;
	//   	使用 uniquePartitionHashes 去边缘节点中进行服务发现;

	// routeId2Indexs := make(map[int64][]*keyToIndex)
	// uniqueShardIds := make([]int64, 0)
	// uniquePartitionHashes := make([]int64, 0)
	// partitionHash2ShardIds := make(map[int64]int64)

	for idx := range kvs {
		kv := kvs[idx]
		shardId, partitionHash, routeToEdgeNode, dc := client.getRouteInfo(*kv.GetKey(), clientOption.ReadMode)

		var groupInfo *dcGroupInfo
		if _, ok := dcGroupInfos[dc]; !ok {
			groupInfo = newDcGroupInfo()
			groupInfo.shardId = shardId
			groupInfo.partitionHash = partitionHash
			groupInfo.routeToEdgeNode = routeToEdgeNode
			dcGroupInfos[dc] = groupInfo
		}
		groupInfo = dcGroupInfos[dc]
		var routerId int64
		if routeToEdgeNode == true {
			routerId = partitionHash
		} else {
			routerId = int64(shardId)
		}
		list, ok := groupInfo.routeId2Indexs[routerId]
		if !ok {
			list = make([]*keyToIndex, 1)
			list[0] = &keyToIndex{
				kv:  &kv,
				idx: idx,
			}
			groupInfo.routeId2Indexs[routerId] = list
		} else {
			list = append(list, &keyToIndex{
				kv:  &kv,
				idx: idx,
			})
			groupInfo.routeId2Indexs[routerId] = list
			continue
		}
		if routeToEdgeNode == true {
			groupInfo.uniquePartitionHashes = append(groupInfo.uniquePartitionHashes, routerId)
			groupInfo.partitionHash2ShardIds[partitionHash] = int64(shardId)
		} else {
			groupInfo.uniqueShardIds = append(groupInfo.uniqueShardIds, routerId)
		}
	}

	for dc, groupInfo := range dcGroupInfos {

		// 服务发现的步骤：
		// 1. 被分配到边缘节点的请求从边缘节点中进行服务发现；
		// 2. 一部分在边缘节点中未发现到服务的请求，重新把它们分配到主集群中(把它们对应的 shard_id 放到 uniqueShardIds 中)。
		// 3. 剩下所有被分配到主集群的请求从主集群中进行服务发现。
		routerOption := client.getClientOption(clientOption, groupInfo.shardId, groupInfo.partitionHash, groupInfo.routeToEdgeNode, dc)
		// 从边缘节点进行服务发现
		routeIdEdgeServerMap := client.router.BatchDiscover(*routerOption, groupInfo.uniquePartitionHashes)
		// 找出从边缘节点服务发现失败的项，分配到主集群中
		for partitionHash, edgeServer := range routeIdEdgeServerMap {
			if nil == edgeServer {
				if _, ok := groupInfo.partitionHash2ShardIds[partitionHash]; !ok {
					continue
				}
				shardId := groupInfo.partitionHash2ShardIds[partitionHash]
				if _, ok := groupInfo.routeId2Indexs[shardId]; !ok {
					groupInfo.uniqueShardIds = append(groupInfo.uniqueShardIds, shardId)
					groupInfo.routeId2Indexs[shardId] = groupInfo.routeId2Indexs[partitionHash]
				} else {
					for _, info := range groupInfo.routeId2Indexs[partitionHash] {
						groupInfo.routeId2Indexs[shardId] = append(groupInfo.routeId2Indexs[shardId], info)
					}
				}
				delete(groupInfo.routeId2Indexs, partitionHash)
				delete(routeIdEdgeServerMap, partitionHash)
			}
		}

		// 从主集群中进行服务发现
		routeIdServerMap := client.router.BatchDiscover(*routerOption, groupInfo.uniqueShardIds)
		for k, v := range routeIdEdgeServerMap {
			routeIdServerMap[k] = v
		}

		for routeId, server := range routeIdServerMap {
			var mapKey string
			if nil == server {
				mapKey = NIL_ADDRESS
			} else {
				mapKey = server.Host + ":" + strconv.Itoa(int(server.Port))
			}
			if _, ok := result[mapKey]; !ok {
				result[mapKey] = &addressKeyMap{
					keyToIndexList: make([]*keyToIndex, 0),
				}
				if nil != server {
					result[mapKey].address = &service_router.ServerAddress{
						Host: server.Host,
						Port: server.Port,
					}
				}
			}
			// 合并不同分片，相同节点的kvs
			result[mapKey].keyToIndexList = append(result[mapKey].keyToIndexList, groupInfo.routeId2Indexs[routeId]...)
		}
	}

	return result
}

type mopType int32

const (
	MOP_ORIGIN mopType = 1
	MOP_DETAIL mopType = 2
)

type commonProcessMop func(callResp *callResp)
type commonCallMset func(c *laser.LaserServiceClient, values []*laser.LaserKV) (*laser.LaserResponse, error)
type commonCallMget func(c *laser.LaserServiceClient, keys []*laser.LaserKey) (*laser.LaserResponse, error)

func (client *LaserClient) mset(clientOption *ClientOption, kvs []laser.LaserKV, callFunc commonCallMset, processFunc commonProcessMop) {
	// 按照address分组
	groupByAddress := client.group(kvs, clientOption)
	wg := sync.WaitGroup{}
	groupSize := len(groupByAddress)
	chs := make(chan *callResp, groupSize)
	wg.Add(groupSize + 1)
	for key, addressMap := range groupByAddress {
		go func(key string, addMap *addressKeyMap, option *ClientOption) {
			length := len(addMap.keyToIndexList)
			values := make([]*laser.LaserKV, length)
			indexes := make([]int, length)
			for idx, val := range addMap.keyToIndexList {
				values[idx] = val.kv
				indexes[idx] = val.idx
			}
			var result *callResp
			if key == NIL_ADDRESS {
				result = &callResp{
					resp: &laser.LaserResponse{
						ListIntData: make([]int64, length),
					},
				}
				for idx := range result.resp.ListIntData {
					result.resp.ListIntData[idx] = -1
				}
			} else {
				option.TargetServerAddress = *addMap.address
				result = client.commonCallWithRouteId(
					0,
					0,
					false,
					service_router.DEFAULT_DC,
					*option,
					func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
						return callFunc(c, values)
					})
			}
			result.idxes = indexes
			chs <- result
			wg.Done()
		}(key, addressMap, clientOption)
	}

	go func() {
		for i := 0; i < groupSize; i++ {
			resp := <-chs
			processFunc(resp)
		}
		wg.Done()
	}()

	wg.Wait()
}

func (client *LaserClient) MsetSync(clientOption ClientOption, kvs []laser.LaserKV) []laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_MSET_TIMER]
	st := timer.Start()
	defer st.Stop()
	clientOption.ReadMode = LEADER_READ
	if len(kvs) == 0 {
		return []laser.Status{}
	}
	result := make([]laser.Status, len(kvs))
	client.mset(&clientOption, kvs, func(c *laser.LaserServiceClient, values []*laser.LaserKV) (*laser.LaserResponse, error) {
		return c.Mset(&laser.LaserKVs{
			Values: values,
		})
	}, func(resp *callResp) {
		listResult := resp.resp.GetListIntData()
		for i, index := range resp.idxes {
			if resp.status != laser.Status_OK {
				result[index] = resp.status
				continue
			}
			res := listResult[i]
			if res == -1 {
				result[index] = laser.Status_CLIENT_THRIFT_CALL_ERROR
			} else {
				result[index] = laser.Status_OK
			}
		}
	})
	return result
}

func (client *LaserClient) MsetDetailSync(clientOption ClientOption, laserSetOpt laser.LaserSetOption, kvs []laser.LaserKV) (laser.Status, []*laser.EntryValue) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_MSET_DETAIL_TIMER]
	st := timer.Start()
	defer st.Stop()
	clientOption.ReadMode = LEADER_READ
	if len(kvs) == 0 {
		return laser.Status_RS_ERROR, []*laser.EntryValue{}
	}
	result := make([]*laser.EntryValue, len(kvs))
	nfailed := 0
	client.mset(&clientOption, kvs, func(c *laser.LaserServiceClient, values []*laser.LaserKV) (*laser.LaserResponse, error) {
		return c.MsetDetail(&laser.LaserKVs{
			Values: values,
		}, &laserSetOpt)
	}, func(resp *callResp) {
		listResult := resp.resp.GetListValueData()
		for i, index := range resp.idxes {
			if resp.status != laser.Status_OK {
				nfailed++
				result[index] = &laser.EntryValue{
					Status:      resp.status,
					StringValue: "",
				}
				continue
			}
			if len(listResult) == 0 {
				nfailed++
				result[index] = &laser.EntryValue{
					Status:      laser.Status_RS_ERROR,
					StringValue: "",
				}
				continue
			}

			status := listResult[i].EntryValue.Status
			failed := status != laser.Status_OK &&
				status != laser.Status_RS_KEY_EXPIRE
			if failed {
				nfailed++
			}
			result[index] = listResult[i].EntryValue
		}
	})
	if nfailed == len(kvs) {
		return laser.Status_RS_ERROR, result
	} else if nfailed > 0 {
		return laser.Status_RS_PART_FAILED, result
	} else {
		return laser.Status_OK, result
	}
}

func (client *LaserClient) mget(clientOption *ClientOption, keys []laser.LaserKey, callFunc commonCallMget, processFunc commonProcessMop) {
	kvs := make([]laser.LaserKV, len(keys))
	for idx := range keys {
		key := &keys[idx]
		kvs[idx] = laser.LaserKV{
			Key:   key,
			Value: nil,
		}
	}
	groupByAddress := client.group(kvs, clientOption)
	groupSize := len(groupByAddress)
	wg := sync.WaitGroup{}
	chs := make(chan *callResp, groupSize)
	wg.Add(groupSize + 1)

	for key, addressMap := range groupByAddress {
		go func(key string, addressMap *addressKeyMap, option *ClientOption) {
			length := len(addressMap.keyToIndexList)
			laserKeys := make([]*laser.LaserKey, length)
			indexes := make([]int, length)
			for idx, val := range addressMap.keyToIndexList {
				laserKeys[idx] = val.kv.Key
				indexes[idx] = val.idx
			}
			var result *callResp
			if key == NIL_ADDRESS {
				result = &callResp{
					status: laser.Status_CLIENT_THRIFT_CALL_ERROR,
					resp:   &laser.LaserResponse{},
				}
			} else {
				option.TargetServerAddress = *addressMap.address
				result = client.commonCallWithRouteId(
					0,
					0,
					false,
					service_router.DEFAULT_DC,
					*option,
					func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
						return callFunc(c, laserKeys)
					})
			}
			result.idxes = indexes
			chs <- result
			wg.Done()
		}(key, addressMap, clientOption)
	}

	go func() {
		for i := 0; i < groupSize; i++ {
			resp := <-chs
			processFunc(resp)
		}
		wg.Done()
	}()
	wg.Wait()
}

func (client *LaserClient) MgetDetailSync(clientOption ClientOption, keys []laser.LaserKey) (laser.Status, []*laser.EntryValue) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_MGET_DETAIL_TIMER]
	st := timer.Start()
	defer st.Stop()
	result := make([]*laser.EntryValue, len(keys))
	nfailed := 0
	client.mget(&clientOption, keys, func(c *laser.LaserServiceClient, keys []*laser.LaserKey) (*laser.LaserResponse, error) {
		return c.MgetDetail(&laser.LaserKeys{
			Keys: keys,
		})
	}, func(resp *callResp) {
		listResult := resp.resp.GetListValueData()
		for i, index := range resp.idxes {
			if resp.status != laser.Status_OK {
				nfailed++
				result[index] = &laser.EntryValue{
					Status:      resp.status,
					StringValue: "",
				}
				continue
			}
			if len(listResult) == 0 {
				nfailed++
				result[index] = &laser.EntryValue{
					Status:      laser.Status_RS_ERROR,
					StringValue: "",
				}
				continue
			}
			status := listResult[i].EntryValue.Status
			failed := status != laser.Status_OK &&
				status != laser.Status_RS_NOT_FOUND &&
				status != laser.Status_RS_KEY_EXPIRE
			if failed {
				nfailed++
			}
			result[index] = listResult[i].EntryValue
		}
	})
	if nfailed == len(keys) {
		return laser.Status_RS_ERROR, result
	} else if nfailed > 0 {
		return laser.Status_RS_PART_FAILED, result
	} else {
		return laser.Status_OK, result
	}
}

func (client *LaserClient) MgetSync(clientOption ClientOption, keys []laser.LaserKey) (laser.Status, []laser.LaserValue) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_MGET_TIMER]
	st := timer.Start()
	result := make([]laser.LaserValue, len(keys))
	status := laser.Status_OK
	client.mget(&clientOption, keys, func(c *laser.LaserServiceClient, keys []*laser.LaserKey) (*laser.LaserResponse, error) {
		return c.Mget(&laser.LaserKeys{
			Keys: keys,
		})
	}, func(resp *callResp) {
		listResult := resp.resp.GetListValueData()
		if resp.status != laser.Status_OK {
			status = resp.status
		} else {
			for i, index := range resp.idxes {
				res := listResult[i]
				result[index] = *res
			}
		}
	})
	st.Stop()
	return status, result
}

func (client *LaserClient) HsetSync(clientOption ClientOption, key laser.LaserKey, field, value string) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_HSET_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Hset(&key, field, value)
	})
	st.Stop()
	return callResp.status
}

func (client *LaserClient) HgetSync(clientOption ClientOption, key laser.LaserKey, field string) (laser.Status, string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_HGET_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Hget(&key, field)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetStringData()
}

func (client *LaserClient) HgetAllSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, map[string]string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_HGETALL_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Hgetall(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetMapStringData()
}

func (client *LaserClient) HexistsSync(clientOption ClientOption, key laser.LaserKey, field string) (laser.Status, bool) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_HEXISTS_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Hexists(&key, field)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetBoolData()
}

func (client *LaserClient) HkeysSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, []string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_HKEYS_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Hkeys(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetListStringData()
}

func (client *LaserClient) HlenSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, int64) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_HLEN_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Hlen(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetIntData()
}

func (client *LaserClient) HdelSync(clientOption ClientOption, key laser.LaserKey, field string) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_HDEL_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Hdel(&key, field)
	})
	st.Stop()
	return callResp.status
}

func (client *LaserClient) HmsetSync(clientOption ClientOption, key laser.LaserKey, val laser.LaserValue) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_HMSET_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Hmset(&key, &val)
	})
	st.Stop()
	return callResp.status
}

func (client *LaserClient) HmgetSync(clientOption ClientOption, key laser.LaserKey, fields []string) (laser.Status, map[string]string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_HMGET_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Hmget(&key, fields)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetMapStringData()
}

func (client *LaserClient) TtlSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, int64) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_TTL_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Ttl(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetIntData()
}

func (client *LaserClient) LpushSync(clientOption ClientOption, key laser.LaserKey, val string) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_LPUSH_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Lpush(&key, val)
	})
	st.Stop()
	return callResp.status
}

func (client *LaserClient) LpopSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_LPOP_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Lpop(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetStringData()
}

func (client *LaserClient) RpushSync(clientOption ClientOption, key laser.LaserKey, val string) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_RPUSH_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Rpush(&key, val)
	})
	st.Stop()
	return callResp.status
}

func (client *LaserClient) RpopSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_RPOP_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Rpop(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetStringData()
}

func (client *LaserClient) LrangeSync(clientOption ClientOption, key laser.LaserKey, start, end int32) (laser.Status, []string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_LRANGE_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Lrange(&key, start, end)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetListStringData()
}

func (client *LaserClient) LindexSync(clientOption ClientOption, key laser.LaserKey, index int32) (laser.Status, string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_LINDEX_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Lindex(&key, index)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetStringData()
}

func (client *LaserClient) LlenSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, int64) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_LLEN_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Llen(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetIntData()
}

func (client *LaserClient) DecrSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, int64) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_DECR_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Decr(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetIntData()
}

func (client *LaserClient) IncrSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, int64) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_INCR_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Incr(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetIntData()
}

func (client *LaserClient) DecrbySync(clientOption ClientOption, key laser.LaserKey, step int64) (laser.Status, int64) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_DECRBY_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.DecrBy(&key, step)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetIntData()
}

func (client *LaserClient) IncrbySync(clientOption ClientOption, key laser.LaserKey, step int64) (laser.Status, int64) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_INCRBY_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.IncrBy(&key, step)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetIntData()
}

func (client *LaserClient) ExistSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, bool) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_EXIST_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Exist(&key)
	})
	st.Stop()
	return callResp.status, callResp.resp.GetBoolData()
}

func (client *LaserClient) ZaddSync(clientOption ClientOption, key laser.LaserKey, member_scores map[string]float64) (laser.Status, int64) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_ZADD_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Zadd(&key, member_scores)
	})
	st.Stop()

	return callResp.status, callResp.resp.GetIntData()
}

func (client *LaserClient) ZrangeByscoreSync(clientOption ClientOption, key laser.LaserKey, min float64, max float64) (laser.Status, []laser.LaserFloatScoreMember) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_ZRANGEBYSCORE_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.ZrangeByScore(&key, min, max)
	})
	st.Stop()
	member_scores := callResp.resp.GetListScoreMemberData()
	ret := make([]laser.LaserFloatScoreMember, 0)
	for _, member_score := range member_scores {
		var info laser.LaserFloatScoreMember
		info.Score = float64(member_score.Score) / 10000
		info.Member = member_score.Member
		ret = append(ret, info)
	}
	return callResp.status, ret
}

func (client *LaserClient) SaddSync(clientOption ClientOption, key laser.LaserKey, member string) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_SADD_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Sadd(&key, member)
	})
	st.Stop()

	return callResp.status
}

func (client *LaserClient) SismemberSync(clientOption ClientOption, key laser.LaserKey, member string) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_SISMEMBER_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Sismember(&key, member)
	})
	st.Stop()

	return callResp.status
}

func (client *LaserClient) SremoveSync(clientOption ClientOption, key laser.LaserKey, member string) laser.Status {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_SREMOVE_TIMER]
	st := timer.Start()
	clientOption.ReadMode = LEADER_READ
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Sremove(&key, member)
	})
	st.Stop()

	return callResp.status
}

func (client *LaserClient) SmembersSync(clientOption ClientOption, key laser.LaserKey) (laser.Status, []string) {
	timer := client.timers[LASER_CLIENT_METRIC_COMMAND_SMEMBERS_TIMER]
	st := timer.Start()
	callResp := client.commonCall(key, clientOption, func(c *laser.LaserServiceClient) (*laser.LaserResponse, error) {
		return c.Smembers(&key)
	})
	st.Stop()

	return callResp.status, callResp.resp.GetListStringData()
}

func (client *LaserClient) GetServiceRouter() *service_router.Router {
	return client.router
}

func NewLaserClient(config *ClientConfig) *LaserClient {
	if strings.TrimSpace(config.ProjectName) == "" {
		panic("please specify project name")
	}
	if strings.TrimSpace(config.LaserServiceName) == "" {
		panic("please specify laser service name")
	}
	if strings.TrimSpace(config.LocalIp) == "" {
		config.LocalIp, _ = ip.GetLocalIPv4Str()
	}
	router := service_router.GetRouter(&service_router.RouterConfig{
		Consul:      config.Consul,
		ProjectName: projectPrefix + strings.TrimSpace(config.ProjectName),
		LocalIp:     config.LocalIp,
		HttpPort:    config.HttpPort,
		Multiplexer: config.Multiplexer,
	})
	client := &LaserClient{
		router:           router,
		localIp:          config.LocalIp,
		laserServiceName: strings.TrimSpace(config.LaserServiceName),
		meters:           make(map[string]tally.Meter),
		timers:           make(map[string]tally.Timer),
		poolMetrics:      make(map[string]tally.Scope),
	}
	client.init()
	return client
}
