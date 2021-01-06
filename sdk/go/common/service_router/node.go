package service_router

import "fmt"

const (
	CONSUL_REGISTRY_KV_PREFIX         string = "ads-core/services/"
	CONSUL_REGISTRY_KV_NODES          string = "/nodes/"
	CONSUL_REGISTRY_KV_CONFIG         string = "/config/"
	CONSUL_REGISTRY_KV_CONFIG_ROUTER  string = "router"
	CONSUL_REGISTRY_KV_CONFIG_CONFIGS string = "configs/"
	CONSUL_REGISTRY_KV_CONFIG_VERSION string = "version"
	CONSUL_REGISTRY_KV_KEY_SPLIT      string = ":"
)

func GetNodeName(server *Server) string {
	// thrift:127.0.0.1:8888
	return fmt.Sprintf("%s%s%s%s%d", server.Protocol, CONSUL_REGISTRY_KV_KEY_SPLIT, server.Host, CONSUL_REGISTRY_KV_KEY_SPLIT, server.Port)
}

func GetNodesPath(server *Server) string {
	// ads-core/services/echo/nodes/
	return CONSUL_REGISTRY_KV_PREFIX + server.ServiceName + CONSUL_REGISTRY_KV_NODES
}

func GetNodePath(server *Server) string {
	// ads-core/services/echo/nodes/thrift:127.0.0.1:8080
	return GetNodesPath(server) + GetNodeName(server)
}

func GetConfigVersionPath(server *Server) string {
	return GetConfigPath(server) + CONSUL_REGISTRY_KV_CONFIG_VERSION
}

func GetConfigPath(server *Server) string {
	// ads-core/services/echo/config/
	return CONSUL_REGISTRY_KV_PREFIX + server.ServiceName + CONSUL_REGISTRY_KV_CONFIG
}

func GetConfigsPath(server *Server) string {
	// ads-core/services/echo/config/configs/
	return GetConfigPath(server) + CONSUL_REGISTRY_KV_CONFIG_CONFIGS
}

func GetRouterConfigPath(server *Server) string {
	// ads-core/services/echo/config/router
	return GetConfigPath(server) + CONSUL_REGISTRY_KV_CONFIG_ROUTER
}

func GetServices(server *Server) string {
	return CONSUL_REGISTRY_KV_PREFIX
}
