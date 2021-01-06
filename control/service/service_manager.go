package service

import (
	"encoding/json"
	"fmt"
	"github.com/hashicorp/consul/api"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
)

const (
	ConsulRegistryKvKeySplit      = ":"
	ConsulRegistryKvPrefix        = "ads-core/services/"
	ConsulRegistryKvConfig        = "/config/"
	ConsulRegistryKvConfigRouter  = "router"
	ConsulRegistryKvConfigConfigs = "configs/"
	ConsulRegistryKvNodes         = "/nodes/"
)

func NewServiceManager(ctx *context.Context) (*Manager, *common.Status) {
	config := &api.Config{
		Address: ctx.GetConfig().ConsulAddress(),
		Token:   ctx.GetConfig().ConsulToken(),
	}

	client, error := api.NewClient(config)
	if error != nil {
		return nil, common.StatusWithError(error)
	}
	instance := &Manager{
		ctx:          ctx,
		consulClient: client,
	}
	return instance, common.StatusOk()
}

type Manager struct {
	ctx          *context.Context
	consulClient *api.Client
}

func (service *Manager) AddService(info params.ServiceAdd) *common.Status {
	hasService, getError := service.hasService(info.ServiceName)
	if getError != nil && !getError.Ok() {
		return getError
	}

	if hasService {
		return common.StatusError(common.ServiceDup)
	}

	routerConfig, _ := json.Marshal(info.RouterConfig)
	kvPair := &api.KVPair{
		Key:   service.getRouterConfigPath(params.ServiceServer{ServiceName: info.ServiceName}),
		Value: routerConfig,
	}
	_, err := service.consulClient.KV().Put(kvPair, &api.WriteOptions{})
	if err != nil {
		return common.StatusWithError(err)
	}
	configsPair := &api.KVPair{
		Key:   service.getConfigsPath(params.ServiceServer{ServiceName: info.ServiceName}),
		Value: nil,
	}
	_, configErr := service.consulClient.KV().Put(configsPair, &api.WriteOptions{})
	if configErr != nil {
		return common.StatusWithError(configErr)
	}
	return common.StatusOk()
}

func (service *Manager) hasService(serviceName string) (bool, *common.Status) {
	pairs, _, err := service.consulClient.KV().List(service.getServicePath(serviceName), &api.QueryOptions{})
	if err != nil {
		return false, common.StatusWithError(err)
	}

	if len(pairs) == 0 {
		return false, common.StatusOk()
	}

	return true, common.StatusOk()
}

func (service *Manager) getServicesPath() string {
	return ConsulRegistryKvPrefix
}

func (service *Manager) getServicePath(serviceName string) string {
	return fmt.Sprintf("%s%s", ConsulRegistryKvPrefix, serviceName)
}

func (service *Manager) getConfigPath(server params.ServiceServer) string {
	return fmt.Sprintf("%s%s%s", ConsulRegistryKvPrefix, server.ServiceName, ConsulRegistryKvConfig)
}

func (service *Manager) getRouterConfigPath(server params.ServiceServer) string {
	return fmt.Sprintf("%s%s", service.getConfigPath(server), ConsulRegistryKvConfigRouter)
}

func (service *Manager) getConfigsPath(server params.ServiceServer) string {
	return fmt.Sprintf("%s%s", service.getConfigPath(server), ConsulRegistryKvConfigConfigs)
}

func (service *Manager) getNodesPath(server params.ServiceServer) string {
	return fmt.Sprintf("%s%s%s", ConsulRegistryKvPrefix, server.ServiceName, ConsulRegistryKvNodes)
}

func (service *Manager) getNodePath(server params.ServiceServer) string {
	return fmt.Sprintf("%s%s", service.getNodesPath(server), service.getNodeName(server))
}

func (service *Manager) getNodeName(server params.ServiceServer) string {
	return fmt.Sprintf("%s%s%s%s%d", server.Protocol, ConsulRegistryKvKeySplit, server.Host, ConsulRegistryKvKeySplit,
		server.Port)
}
