package service_router

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"reflect"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/algo-data-platform/LaserDB/sdk/go/common/ip"
	"github.com/hashicorp/consul/api"
	"github.com/liubang/tally"
	promreporter "github.com/liubang/tally/prometheus"
	"github.com/zentures/cityhash"
)

const (
	LoadBalanceMethod_RANDOM              = "random"
	LoadBalanceMethod_ROUNDROBIN          = "roundrobin"
	LoadBalanceMethod_LOCALFIRST          = "localfirst"
	LoadBalanceMethod_CONSISTENT          = "consistent"
	LoadBalanceMethod_CONFIGURABLE_WEIGHT = "configurable_weight"
	LoadBalanceMethod_ACTIVE_WEIGHT       = "active_weight"
	LoadBalanceMethod_USER_DEFINED        = "user_defined"
	LoadBalanceMethod_IPRANGEFIRST        = "iprangefirst"
	LoadBalanceMethod_STATIC_WEIGHT       = "static_weight"
)

var SupportedLoadBalanceMethod = map[string]bool{
	LoadBalanceMethod_RANDOM:              true,
	LoadBalanceMethod_ROUNDROBIN:          true,
	LoadBalanceMethod_LOCALFIRST:          true,
	LoadBalanceMethod_CONSISTENT:          false,
	LoadBalanceMethod_CONFIGURABLE_WEIGHT: true,
	LoadBalanceMethod_ACTIVE_WEIGHT:       false,
	LoadBalanceMethod_IPRANGEFIRST:        true,
	LoadBalanceMethod_STATIC_WEIGHT:       true,
	LoadBalanceMethod_USER_DEFINED:        false,
}

const (
	ShardType_ALL      = "0"
	ShardType_LEADER   = "1"
	ShardType_FOLLOWER = "2"
)

type httpResult struct {
	Code    int32       `json:"code"`
	Message string      `json:"message"`
	Data    interface{} `json:"data"`
}

const UINT32MAX int64 = 4294967295 // 2^32 - 1

type BalanceLocalFirstConfig struct {
	LocalIp   string
	DiffRange int
}

type ServerAddress struct {
	Host string `json:"Host" toml:"Host"`
	Port uint16 `json:"Port" toml:"Port"`
}

type ClientOption struct {
	ServiceName         string
	ShardId             int64
	ShardType           string
	Protocol            string
	Loadbalance         string
	TargetServerAddress ServerAddress
	UserBalance         LoadBalance
	LocalFirstConfig    BalanceLocalFirstConfig
	PartitionHash       int64
	RouteToEdgeNode     bool
	Idc                 string
	Dc                  string
}

type ServiceRouterConfig struct {
	TtlInMs           uint32 `json:"TtlInMs" toml:"TtlInMs"`
	LoadBalanceMethod string `json:"LoadBalance" toml:"LoadBalance"`
	TotalShards       uint32 `json:"TotalShards" toml:"TotalShards"`
	PullInterval      uint32 `json:"PullInterval" toml:"PullInterval"`
}

type ServiceConfig struct {
	Router  ServiceRouterConfig
	Configs map[string]string
}

type Multiplexer func(string, http.Handler)

type RouterConfig struct {
	Consul      api.Config
	ProjectName string
	LocalIp     string
	HttpPort    int
	Multiplexer Multiplexer
}

// Get default ServiceRouterConfig
func DefaultServiceRouterConfig() ServiceRouterConfig {
	return ServiceRouterConfig{
		TtlInMs:           300000,
		LoadBalanceMethod: LoadBalanceMethod_RANDOM,
		TotalShards:       0,
		PullInterval:      3,
	}
}

func ClientOptionFactory(serviceName string, protocol string) ClientOption {
	config := DefaultClientOption()
	config.ServiceName = serviceName
	config.Protocol = protocol
	return config
}

// Get Default ClientOption
func DefaultClientOption() ClientOption {
	return ClientOption{
		ShardId:          UINT32MAX,
		ShardType:        ShardType_LEADER,
		Loadbalance:      LoadBalanceMethod_LOCALFIRST,
		LocalFirstConfig: DefaultBalanceLocalFirstConfig(),
		Dc:               DEFAULT_DC,
	}
}

// Get default BalanceLocalFirstConfig
func DefaultBalanceLocalFirstConfig() BalanceLocalFirstConfig {
	ip, _ := ip.GetLocalIPv4Str()
	return BalanceLocalFirstConfig{
		LocalIp:   ip,
		DiffRange: 256,
	}
}

type Router struct {
	registry        Registry
	routerDb        RouterDb
	metrics         tally.Scope
	counters        map[string]tally.Counter
	configPullers   map[string]*ServicePuller
	discoverPullers map[string]*ServicePuller
	servicePushers  map[string]*ServicePusher
	balancers       map[uint64]LoadBalance
	counterLock     sync.RWMutex
	configLock      sync.RWMutex
	discoverLock    sync.RWMutex
	pusherLock      sync.RWMutex
	balancerLock    sync.RWMutex
}

var (
	routerInstance *Router
	routerOnce     sync.Once
)

func (router *Router) getServiceKey(server Server) string {
	return server.ServiceName + "::" + server.Host + ":" + strconv.Itoa(int(server.Port))
}

func (router *Router) GetOrCreateServicePusher(server Server) *ServicePusher {
	key := router.getServiceKey(server)
	router.pusherLock.RLock()
	sh, ok := router.servicePushers[key]
	router.pusherLock.RUnlock()
	if !ok {
		// upgrade
		router.pusherLock.Lock()
		defer router.pusherLock.Unlock()
		sh, ok = router.servicePushers[key]
		if !ok {
			pusher := NewConsulServicePush(router.registry)
			routerConfig, _ := router.GetServiceRouterConfig(server.ServiceName)
			sh = NewServicePusher(server, pusher, *routerConfig)
			router.servicePushers[key] = sh
			router.registry.SubscribeConfig(server.ServiceName, router)
		}
	}
	return sh
}

// update timer
func (router *Router) ConfigNotify(serviceName string, serviceConfig ServiceConfig) {
	if serviceName == "" {
		return
	}
	interval := calculateInterval(serviceConfig.Router.PullInterval)
	if interval > 0 {
		router.configLock.RLock()
		configPuller, hasConfigPuller := router.configPullers[serviceName]
		router.configLock.RUnlock()

		router.discoverLock.RLock()
		servicePuller, hasServicePuller := router.discoverPullers[serviceName]
		router.discoverLock.RUnlock()

		if hasConfigPuller && configPuller.interval != interval {
			configPuller.Reload(interval)
		}

		if hasServicePuller && servicePuller.interval != interval {
			servicePuller.Reload(interval)
		}
	}
	ttl := calculateTtl(serviceConfig.Router.TtlInMs)
	if ttl > 0 {
		router.pusherLock.RLock()
		servicePusher, hasServicePusher := router.servicePushers[serviceName]
		router.pusherLock.RUnlock()
		if hasServicePusher && servicePusher.ttl != ttl {
			servicePusher.Reload(ttl)
		}
	}
}

func (router *Router) GetOrCreateConfigPuller(serviceName string) *ServicePuller {
	router.configLock.RLock()
	puller, ok := router.configPullers[serviceName]
	router.configLock.RUnlock()
	if !ok {
		// upgrade
		router.configLock.Lock()
		defer router.configLock.Unlock()
		puller, ok = router.configPullers[serviceName]
		if !ok {
			// subscribe routerdb
			router.registry.SubscribeConfig(serviceName, router.routerDb)
			serviceConfigPull := GetConsulServiceConfigPull(router.registry)
			serviceConfigPull.Pull(serviceName)
			routerConfig := router.routerDb.GetServiceRouterConfig(serviceName)
			puller = NewServicePuller(serviceName, serviceConfigPull, routerConfig, ServicePullerType_Config)
			// subscribe self
			router.configPullers[serviceName] = puller
			router.registry.SubscribeConfig(serviceName, router)
		}
	}
	return puller
}

func (router *Router) GetOrCreateDiscoverPuller(serviceName string) *ServicePuller {
	router.discoverLock.RLock()
	puller, ok := router.discoverPullers[serviceName]
	router.discoverLock.RUnlock()
	if !ok {
		// upgrade
		router.discoverLock.Lock()
		defer router.discoverLock.Unlock()
		puller, ok = router.discoverPullers[serviceName]
		if !ok {
			// subscribe
			router.registry.SubscribeService(serviceName, router.routerDb)
			serviceInfoPull := GetConsulServiceInfoPull(router.registry)
			routerConfig, _ := router.GetServiceRouterConfig(serviceName)
			puller = NewServicePuller(serviceName, serviceInfoPull, *routerConfig, ServicePullerType_Server)
			router.discoverPullers[serviceName] = puller
		}
	}
	return puller
}

func (router *Router) GetOrCreateBalancer(option ClientOption) LoadBalance {
	var (
		balanceId   uint64
		method      string      = option.Loadbalance
		protocol    string      = option.Protocol
		serviceName string      = option.ServiceName
		userBalance LoadBalance = option.UserBalance
	)
	balanceId = cityhash.CityHash64WithSeed([]byte(method), uint32(len(method)), 0)
	balanceId = cityhash.CityHash64WithSeed([]byte(serviceName), uint32(len(serviceName)), balanceId)
	balanceId = cityhash.CityHash64WithSeed([]byte(protocol), uint32(len(protocol)), balanceId)
	if method == LoadBalanceMethod_USER_DEFINED {
		if nil != userBalance {
			customBalancerName := reflect.TypeOf(userBalance).Name()
			balanceId = cityhash.CityHash64WithSeed([]byte(customBalancerName), uint32(len(customBalancerName)), balanceId)
		} else {
			method = LoadBalanceMethod_RANDOM
		}
	}
	router.balancerLock.RLock()
	balancer, ok := router.balancers[balanceId]
	router.balancerLock.RUnlock()
	if !ok {
		router.balancerLock.Lock()
		defer router.balancerLock.Unlock()
		balancer, ok = router.balancers[balanceId]
		if !ok {
			switch method {
			case LoadBalanceMethod_RANDOM:
				balancer = NewLoadBalanceRandom()
			case LoadBalanceMethod_ROUNDROBIN:
				balancer = NewLoadBalanceRoundrobin()
			case LoadBalanceMethod_LOCALFIRST:
				balancer = NewLoadBalanceLocalFirst(option.LocalFirstConfig)
			case LoadBalanceMethod_IPRANGEFIRST:
				balancer = NewLoadBalanceIpRangeFirst(option.LocalFirstConfig)
			case LoadBalanceMethod_STATIC_WEIGHT:
				balancer = NewLoadBalanceStaticWeight()
			case LoadBalanceMethod_CONFIGURABLE_WEIGHT:
				balancer = NewLoadBalanceConfigurableWeight()
			case LoadBalanceMethod_USER_DEFINED:
				balancer = userBalance
			default:
				balancer = NewLoadBalanceRandom()
			}
			router.balancers[balanceId] = balancer
		}
	}
	return balancer
}

func (router *Router) Discover(option ClientOption) (*Server, bool) {
	var routeId int64
	router.GetOrCreateDiscoverPuller(option.ServiceName)
	if len(option.Dc) == 0 {
		option.Dc = DEFAULT_DC
	}
	if option.RouteToEdgeNode {
		routeId = option.PartitionHash
	} else {
		routeId = option.ShardId
	}
	serverList, ok := router.routerDb.SelectServers(option.ServiceName, option.Protocol, routeId, option.ShardType, option.Dc)
	if !ok && option.RouteToEdgeNode {
		serverList, ok = router.routerDb.SelectServers(option.ServiceName, option.Protocol, option.ShardId, option.ShardType, option.Dc)
	}
	if !ok {
		counter := router.GetCounter(nil)
		counter.Inc(1)
		return nil, false
	}
	balancer := router.GetOrCreateBalancer(option)
	res, ok := balancer.Select(serverList)
	counter := router.GetCounter(res)
	counter.Inc(1)
	return res, ok
}

func getServerListHash(serverList ServerList) uint64 {
	var result uint64 = 0
	for idx := range serverList {
		server := &serverList[idx]
		str := server.Host + strconv.Itoa(int(server.Port))
		result = cityhash.CityHash64WithSeed([]byte(str), uint32(len(str)), result)
	}
	return result
}

// 通过shardIds批量发现
func (router *Router) BatchDiscover(option ClientOption, routerIds []int64) map[int64]*Server {
	router.GetOrCreateDiscoverPuller(option.ServiceName)
	if len(option.Dc) == 0 {
		option.Dc = DEFAULT_DC
	}
	serverListsMap := router.routerDb.BatchSelectServers(option.ServiceName, option.Protocol, routerIds, option.ShardType, option.Dc)
	balancer := router.GetOrCreateBalancer(option)
	result := make(map[int64]*Server)
	selectServers := make(map[uint64]*Server)
	for _, routeId := range routerIds {
		if _, ok := serverListsMap[routeId]; ok {
			serverList := serverListsMap[routeId]
			hashCode := getServerListHash(serverList)
			if _, ok := selectServers[hashCode]; !ok {
				server, ok := balancer.Select(serverList)
				if ok {
					selectServers[hashCode] = server
				} else {
					selectServers[hashCode] = nil
				}
			}
			result[routeId] = selectServers[hashCode]
		} else {
			result[routeId] = nil
		}
	}
	return result
}

func (router *Router) GetServerList(option ClientOption) (ServerList, bool) {
	router.GetOrCreateDiscoverPuller(option.ServiceName)
	var routerId int64
	if option.RouteToEdgeNode {
		routerId = option.PartitionHash
	} else {
		routerId = option.ShardId
	}
	serverList, ok := router.routerDb.SelectServers(option.ServiceName, option.Protocol, routerId, option.ShardType, option.Dc)
	if !ok {
		return nil, false
	}
	return serverList, ok
}

func (router *Router) GetServiceRouterConfig(serviceName string) (*ServiceRouterConfig, bool) {
	router.GetOrCreateConfigPuller(serviceName)
	config := router.routerDb.GetServiceRouterConfig(serviceName)
	return &config, true
}

func (router *Router) GetConfigs(name string) (map[string]string, bool) {
	router.GetOrCreateConfigPuller(name)
	return router.routerDb.GetServiceConfig(name)
}

func (router *Router) SetRouterDb(db RouterDb) {
	router.routerDb = db
}

func (router *Router) setIsEdgeNode(server *Server, isEdgeNode bool) {
	server.IsEdgeNode = isEdgeNode
}

func (router *Router) setPartitionList(server *Server, partitionList []int64) {
	for _, partition := range partitionList {
		server.PartitionList = append(server.PartitionList, partition)
	}
}

func (router *Router) GetRouterDb() RouterDb {
	return router.routerDb
}

func (router *Router) RegisterServer(server Server) {
	router.GetOrCreateServicePusher(server)
}

func (router *Router) GetRegistry() Registry {
	return router.registry
}

func (router *Router) GetCounter(server *Server) tally.Counter {
	var key string
	if nil == server {
		key = ROUTER_METRICS_SELECT_ADDRESS_TAGS_ADDR_VAL_NONE
	} else {
		key = server.Host + ":" + strconv.Itoa(int(server.Port))
	}
	router.counterLock.RLock()
	counter, ok := router.counters[key]
	router.counterLock.RUnlock()
	if !ok {
		router.counterLock.Lock()
		counter, ok = router.counters[key]
		if !ok {
			tags := map[string]string{ROUTER_METRICS_SELECT_ADDRESS_TAGS_ADDR: key}
			counter = router.metrics.Tagged(tags).Counter(ROUTER_METRICS_SELECT_ADDRESS)
			router.counters[key] = counter
		}
		router.counterLock.Unlock()
	}
	return counter
}

func (router *Router) GetAllSubscribed() http.Handler {
	return http.HandlerFunc(func(rsp http.ResponseWriter, _ *http.Request) {
		subscribed := router.registry.GetAllSubscribed()
		result := httpResult{
			Code:    0,
			Message: "",
			Data:    subscribed,
		}
		str, _ := json.Marshal(result)
		rsp.Write(str)
	})
}

func (router *Router) StaticSwitch() http.Handler {
	return http.HandlerFunc(func(rsp http.ResponseWriter, req *http.Request) {
		query := req.URL.Query()
		serviceName, serviceNameExist := query["service_name"]
		useStatic, useStaticExist := query["use_static"]
		var result httpResult
		if !serviceNameExist || len(serviceName) == 0 || !useStaticExist || len(useStatic) == 0 {
			result.Code = 1001
			result.Message = "Parameter error."
			result.Data = ""
		} else {
			if useStatic[0] == "0" {
				router.registry.DisableStatic(serviceName[0])
			} else {
				router.registry.EnableStatic(serviceName[0])
			}
			result.Code = 0
			result.Message = ""
			result.Data = ""
		}
		str, _ := json.Marshal(result)
		rsp.Write(str)
	})
}

func (router *Router) PushStaticNodes() http.Handler {
	return http.HandlerFunc(func(rsp http.ResponseWriter, req *http.Request) {
		query := req.URL.Query()
		serviceName, serviceNameExist := query["service_name"]
		var result httpResult
		if !serviceNameExist || len(serviceName) == 0 || len(serviceName[0]) == 0 {
			result.Code = 1001
			result.Message = "Parameter error."
			result.Data = ""
		} else {
			s, _ := ioutil.ReadAll(req.Body)
			var serverList []Server
			err := json.Unmarshal(s, &serverList)
			if err != nil {
				result.Code = 1002
				result.Message = err.Error()
				result.Data = ""
			} else {
				router.registry.PushStaticNodes(serviceName[0], serverList)
				result.Code = 0
				result.Message = "Success."
				result.Data = ""
			}
		}
		str, _ := json.Marshal(result)
		rsp.Write(str)
	})
}

func (router *Router) RestartTimerHandler() http.Handler {
	return http.HandlerFunc(func(rsp http.ResponseWriter, _ *http.Request) {
		result := make([]string, 2)
		router.configLock.Lock()
		for k, v := range router.configPullers {
			v.Restart()
			result = append(result, fmt.Sprintf("[config_puller] %s", k))
		}
		router.configLock.Unlock()

		router.discoverLock.Lock()
		for k, v := range router.discoverPullers {
			v.Restart()
			result = append(result, fmt.Sprintf("[discover_puller] %s", k))
		}
		router.discoverLock.Unlock()

		router.pusherLock.Lock()
		for k, v := range router.servicePushers {
			v.Restart()
			result = append(result, fmt.Sprintf("[service_heartbeat] %s", k))
		}
		router.pusherLock.Unlock()
		str, _ := json.Marshal(result)
		rsp.Write(str)
	})
}

func (router *Router) GetMetrics() tally.Scope {
	return router.metrics
}

func (router *Router) initHttp(config *RouterConfig, reporter promreporter.Reporter) {
	var port = config.HttpPort
	if port < 0 {
		port = 0
	}
	if nil == config.Multiplexer {
		serveMux := &http.ServeMux{}
		listen, _ := net.Listen("tcp", fmt.Sprintf("0.0.0.0:%d", port))
		if port == 0 {
			port = listen.Addr().(*net.TCPAddr).Port
		}
		serveMux.Handle(METRICS_URI, reporter.JsonHTTPHandler())
		serveMux.Handle(PROMETHEUS_URI, reporter.HTTPHandler())
		serveMux.Handle(RESTART_TIMER, router.RestartTimerHandler())
		serveMux.Handle(HTTP_SERVER_ROUTER_PUT_NODES, router.PushStaticNodes())
		serveMux.Handle(HTTP_SERVER_ROUTER_STATIC_SWITCH, router.StaticSwitch())
		serveMux.Handle(HTTP_SERVER_SUBSCRIBED, router.GetAllSubscribed())
		http := &http.Server{Handler: serveMux}
		go http.Serve(listen)
	} else {
		config.Multiplexer(METRICS_URI, reporter.JsonHTTPHandler())
		config.Multiplexer(PROMETHEUS_URI, reporter.HTTPHandler())
		config.Multiplexer(RESTART_TIMER, router.RestartTimerHandler())
		config.Multiplexer(HTTP_SERVER_ROUTER_PUT_NODES, router.PushStaticNodes())
		config.Multiplexer(HTTP_SERVER_ROUTER_STATIC_SWITCH, router.StaticSwitch())
		config.Multiplexer(HTTP_SERVER_SUBSCRIBED, router.GetAllSubscribed())
	}
	// registry http server
	server := Server{
		ServiceName: config.ProjectName,
		Host:        config.LocalIp,
		Port:        uint16(port),
		Protocol:    ServerProtocol_HTTP,
		Status:      ServerStatus_AVAILABLE,
	}
	router.RegisterServer(server)
	if _, ok := router.registry.GetValue(GetRouterConfigPath(&server)); !ok {
		// registry router config
		router.registry.PutRouterConfig(server, DefaultServiceRouterConfig())
	}
}

// singleton
func GetRouter(config *RouterConfig) *Router {
	routerOnce.Do(func() {
		if strings.TrimSpace(config.LocalIp) == "" {
			config.LocalIp, _ = ip.GetLocalIPv4Str()
		}
		registry, _ := NewConsulRegistry(&config.Consul)
		routerDb := GetRouterDb()
		reporter := promreporter.NewReporter(promreporter.Options{})
		scope, _ := tally.NewRootScope(tally.ScopeOptions{
			Prefix:         PREFIX,
			CachedReporter: reporter,
			Separator:      promreporter.DefaultSeparator,
			Tags:           map[string]string{"project": config.ProjectName},
		}, time.Second)
		routerInstance = &Router{
			routerDb:        routerDb,
			registry:        registry,
			metrics:         scope,
			counters:        make(map[string]tally.Counter),
			discoverPullers: make(map[string]*ServicePuller),
			configPullers:   make(map[string]*ServicePuller),
			servicePushers:  make(map[string]*ServicePusher),
			balancers:       make(map[uint64]LoadBalance),
		}
		routerInstance.initHttp(config, reporter)
	})
	return routerInstance
}
