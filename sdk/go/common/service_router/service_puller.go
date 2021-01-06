package service_router

import (
	"context"
	"log"
	"sync"
	"time"

	"go.uber.org/atomic"
)

// service pull interface
type ServicePull interface {
	Pull(serviceName string)
}

type ConsulServiceInfoPull struct {
	registry Registry
}

var (
	serviceInfoPull *ConsulServiceInfoPull
	sipOnce         sync.Once
)

// singleton
func GetConsulServiceInfoPull(registry Registry) *ConsulServiceInfoPull {
	sipOnce.Do(func() {
		serviceInfoPull = &ConsulServiceInfoPull{
			registry: registry,
		}
	})

	return serviceInfoPull
}

func (sip *ConsulServiceInfoPull) Pull(serviceName string) {
	sip.registry.Discover(serviceName)
}

type ConsulServiceConfigPull struct {
	registry Registry
}

var (
	serviceConfigPull *ConsulServiceConfigPull
	scpOnce           sync.Once
)

func GetConsulServiceConfigPull(registry Registry) *ConsulServiceConfigPull {
	scpOnce.Do(func() {
		serviceConfigPull = &ConsulServiceConfigPull{
			registry: registry,
		}
	})

	return serviceConfigPull
}

func (scp *ConsulServiceConfigPull) Pull(serviceName string) {
	scp.registry.GetConfig(serviceName)
}

const (
	ServicePullerType_Config = "CONFIG_PULLER"
	ServicePullerType_Server = "SERVER_PULLER"
)

type ServicePuller struct {
	serviceName string
	pullerType  string
	interval    time.Duration
	servicePull ServicePull
	ticker      *time.Ticker
	cancel      context.CancelFunc
	started     *atomic.Bool
}

func calculateInterval(interval uint32) time.Duration {
	return time.Duration(2*interval*1000/3) * time.Millisecond
}

func NewServicePuller(serviceName string, servicePull ServicePull, routerConfig ServiceRouterConfig, pullerType string) *ServicePuller {
	ctx, cancel := context.WithCancel(context.Background())
	puller := &ServicePuller{
		interval:    calculateInterval(routerConfig.PullInterval),
		pullerType:  pullerType,
		serviceName: serviceName,
		servicePull: servicePull,
		cancel:      cancel,
		started:     atomic.NewBool(false),
	}
	puller.Start(ctx)
	return puller
}

func (sp *ServicePuller) Start(ctx context.Context) {
	if !sp.started.CAS(false, true) {
		return
	}
	// first do task, then create ticker
	sp.servicePull.Pull(sp.serviceName)
	ticker := time.NewTicker(sp.interval)
	sp.ticker = ticker
	log.Printf("[%s] - %s ticker created with interval %v\n", sp.pullerType, sp.serviceName, sp.interval)
	go func(serviceName string, servicePull ServicePull) {
		defer ticker.Stop()
		for {
			select {
			case <-ticker.C:
				go servicePull.Pull(serviceName)
			case <-ctx.Done():
				log.Printf("[%s] - %s ticker stopped\n", sp.pullerType, sp.serviceName)
				return
			}
		}
	}(sp.serviceName, sp.servicePull)
}

func (sp *ServicePuller) Restart() {
	sp.Stop()
	ctx, cancel := context.WithCancel(context.Background())
	sp.cancel = cancel
	sp.Start(ctx)
}

func (sp *ServicePuller) Reload(interval time.Duration) {
	sp.Stop()
	sp.interval = interval
	ctx, cancel := context.WithCancel(context.Background())
	sp.cancel = cancel
	sp.Start(ctx)
}

func (sp *ServicePuller) Stop() {
	sp.cancel()
	sp.started.Store(false)
}
