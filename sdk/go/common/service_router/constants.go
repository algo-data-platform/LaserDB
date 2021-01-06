package service_router

const (
	PREFIX                                           = "ads_core_go"
	NAMESPACE                                        = "service_router"
	ROUTER_VERSION                                   = "service_router_version"
	ROUTER_MAJOR_VERSION                             = "major_version"
	ROUTER_MINOR_VERSION                             = "minor_version"
	ROUTER_REVISION                                  = "revision"
	ROUTER_METRICS_SELECT_ADDRESS                    = "select_address"
	ROUTER_METRICS_SELECT_ADDRESS_TAGS_ADDR          = "addr"
	ROUTER_METRICS_SELECT_ADDRESS_TAGS_ADDR_VAL_NONE = "none"
	POOL_METRICS_AQUIRE_BLOCKED                      = "pool_aquire_blocked"
	POOL_IDLE_CONNECTIONS                            = "pool_idle_connections"
	POOL_ACTIVE_CONNECTIONS                          = "pool_active_connections"
	METRICS_URI                                      = "/ads-core/metrics"
	PROMETHEUS_URI                                   = "/ads-core/metrics/prometheus"
	RESTART_TIMER                                    = "/ads-core/timer/restart"
	HTTP_SERVER_SUBSCRIBED                           = "/ads-core/router/subscribed"
	HTTP_SERVER_ROUTER_STATIC_SWITCH                 = "/ads-core/router/static_switch"
	HTTP_SERVER_ROUTER_PUT_NODES                     = "/ads-core/router/put_nodes"
)
