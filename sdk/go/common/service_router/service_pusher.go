package service_router

import (
	"context"
	"log"
	"sync"
	"time"

	"go.uber.org/atomic"
)

type ServicePush interface {
	Push(server Server) error
}

type ConsulServicePush struct {
	registry Registry
}

var (
	consulServicePusherOnce     sync.Once
	consulServicePusherInstance *ConsulServicePush
)

func NewConsulServicePush(registry Registry) *ConsulServicePush {
	consulServicePusherOnce.Do(func() {
		consulServicePusherInstance = &ConsulServicePush{
			registry: registry,
		}
	})
	return consulServicePusherInstance
}

func (csp *ConsulServicePush) Push(server Server) error {
	return csp.registry.RegisterServer(server)
}

type ServicePusher struct {
	server  Server
	push    ServicePush
	ticker  *time.Ticker
	ttl     time.Duration
	cancel  context.CancelFunc
	started *atomic.Bool
}

func calculateTtl(ttl uint32) time.Duration {
	return time.Duration(2*ttl/3) * time.Millisecond
}

func NewServicePusher(server Server, push ServicePush, routerConfig ServiceRouterConfig) *ServicePusher {
	ctx, cancel := context.WithCancel(context.Background())
	sh := &ServicePusher{
		server:  server,
		push:    push,
		ttl:     calculateTtl(routerConfig.TtlInMs),
		cancel:  cancel,
		started: atomic.NewBool(false),
	}
	sh.Start(ctx)
	return sh
}

func (sh *ServicePusher) Start(ctx context.Context) {
	if !sh.started.CAS(false, true) {
		return
	}
	// first do task, then create ticker
	sh.push.Push(sh.server)
	ticker := time.NewTicker(sh.ttl)
	sh.ticker = ticker
	log.Printf("[Heartbeat] - %s ticker created with ttl %v\n", sh.server.ServiceName, sh.ttl)
	go func(ticker *time.Ticker, push ServicePush, server *Server) {
		defer ticker.Stop()
		for {
			select {
			case <-ticker.C:
				go push.Push(*server)
			case <-ctx.Done():
				log.Printf("[Heartbeat] - %s ticker stopped\n", sh.server.ServiceName)
				return
			}
		}
	}(ticker, sh.push, &sh.server)
}

func (sh *ServicePusher) Restart() {
	sh.Stop()
	ctx, cancel := context.WithCancel(context.Background())
	sh.cancel = cancel
	sh.Start(ctx)
}

func (sh *ServicePusher) Reload(ttl time.Duration) {
	sh.Stop()
	ctx, cancel := context.WithCancel(context.Background())
	sh.ttl = ttl
	sh.cancel = cancel
	sh.Start(ctx)
}

func (sh *ServicePusher) Stop() {
	sh.cancel()
	sh.started.Store(false)
}

func (sh *ServicePusher) SetWeight(weight uint32) {
	sh.server.Weight = weight
}

func (sh *ServicePusher) SetShardList(shardList []uint32) {
	sh.server.ShardList = shardList
}

func (sh *ServicePusher) SetAvailableShardList(shardList []uint32) {
	sh.server.AvailableShardList = shardList
}

func (sh *ServicePusher) SetFollowerShardList(shardList []uint32) {
	sh.server.FollowerShardList = shardList
}

func (sh *ServicePusher) SetFollowerAvailableShardList(shardList []uint32) {
	sh.server.FollowerAvailableShardList = shardList
}

func (sh *ServicePusher) SetStatus(status string) {
	sh.server.Status = status
}

func (sh *ServicePusher) SetIsEdgeNode(isEdgeNode bool) {
	sh.server.IsEdgeNode = isEdgeNode
}

func (sh *ServicePusher) SetPartitionList(partition_list []int64) {
	for _, partition := range partition_list {
		sh.server.PartitionList = append(sh.server.PartitionList, partition)
	}
}

