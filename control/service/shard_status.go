package service

import (
	"context"
	"encoding/json"
	"math"
	"sync"
	"time"

	laserContext "laser-control/context"
)

const (
	DefaultDiffRate       = 3
	DefaultDiffSeqNumbers = 1000
)

type ShardServiceState uint32

type ShardState uint32

const (
	LeaderStatus         ShardState = iota + 1 // 当前分区处于 Leader 状态
	LeaderTempStatus                           // 当前分片原本是 Follower 状态，临时切换为 Leader
	FollowerStatus                             // 当前分片为 Follower 状态
	FollowerCreateStatus                       // 当前分片刚刚创建，数据还没有同步完成，同步完成切换为 Follower 状态
	FollowerDeleteStatus                       // 当前
)

type ShardStatus struct {
	ShardId         uint32            `json:"shardId"`
	GroupName       string            `json:"groupName"`
	NodeId          uint32            `json:"nodeId"`
	Role            ShardRole         `json:"role"`
	SeqNo           uint64            `json:"seqNo"`
	BaseVersionHash uint64            `json:"baseVersionHash"`
	ServiceState    ShardServiceState `json:"serviceState"`
	ShardHash       string            `json:"shardHash"`
	NoDiff          bool              `json:"noDiff"`
	HasLeader       bool              `json:"hasLeader"`
}

type NodeHostInfo struct {
	GroupName string
	NodeId    uint32
	Host      string
	Port      uint16
}

type ShardNodeGroup map[uint32]ShardStatus

type ShardPullResult struct {
	nodeHash uint64
	shards   ShardNodeGroup
}

type ShardRelation struct {
	Leader   string   `json:"leader"`
	Follower []string `json:"follower"`
}

type ShardStatusManager struct {
	sync.Mutex
	ctx                 *laserContext.Context
	shards              map[string]ShardStatus
	shardRelations      map[uint32]ShardRelation
	nodes               map[uint64]*NodeHostInfo
	shardMetricsManager *ShardMetricsManager
	cancel              context.CancelFunc
	ttlMs               uint32
}

func NewShardStatusManager(ctx *laserContext.Context) *ShardStatusManager {
	instance := &ShardStatusManager{
		Mutex:               sync.Mutex{},
		ctx:                 ctx,
		shards:              make(map[string]ShardStatus),
		nodes:               make(map[uint64]*NodeHostInfo),
		shardMetricsManager: GetShardMetricsManager(ctx),
		ttlMs:               5000,
	}
	instance.start()
	return instance
}

func (shardStatusManager *ShardStatusManager) SetNodes(nodes []*NodeHostInfo) {
	newNodes := make(map[uint64]*NodeHostInfo)
	for _, node := range nodes {
		newNodes[shardStatusManager.getNodeHash(node)] = node
	}
	shardStatusManager.Lock()
	defer shardStatusManager.Unlock()
	shardStatusManager.nodes = newNodes
}

func (shardStatusManager *ShardStatusManager) AddNodes(nodes []*NodeHostInfo) {
	shardStatusManager.Lock()
	defer shardStatusManager.Unlock()
	for _, node := range nodes {
		shardStatusManager.nodes[shardStatusManager.getNodeHash(node)] = node
	}
}

func (shardStatusManager *ShardStatusManager) DeleteNodes(nodes []*NodeHostInfo) {
	shardStatusManager.Lock()
	defer shardStatusManager.Unlock()
	for _, node := range nodes {
		delete(shardStatusManager.nodes, shardStatusManager.getNodeHash(node))
	}
}

func (shardStatusManager *ShardStatusManager) GetShardStatus() (map[string]ShardStatus, map[uint32]ShardRelation) {
	shards := make(map[string]ShardStatus)
	relations := make(map[uint32]ShardRelation)
	shardStatusManager.Lock()
	shardsStr, _ := json.Marshal(shardStatusManager.shards)
	relationsStr, _ := json.Marshal(shardStatusManager.shardRelations)
	shardStatusManager.Unlock()
	_ = json.Unmarshal(shardsStr, &shards)
	_ = json.Unmarshal(relationsStr, &relations)

	return shards, relations
}

func (shardStatusManager *ShardStatusManager) UpdateDone() bool {
	// TODO 校验数据库 FollowerCreateStatus 状态分片是否同步完成
	return false
}

func (shardStatusManager *ShardStatusManager) getNodeHash(node *NodeHostInfo) uint64 {
	return GetNodeHash(node.GroupName, node.NodeId)
}

func (shardStatusManager *ShardStatusManager) start() {
	ctx, cancel := context.WithCancel(context.Background())
	shardStatusManager.cancel = cancel
	ttl := time.Duration(shardStatusManager.ttlMs) * time.Millisecond
	ticker := time.NewTicker(ttl)
	time.Sleep(time.Duration(10) * time.Second)
	go func() {
		for {
			select {
			case <-ticker.C:
				shardStatusManager.ctx.Log().Debug("Start pull shard list")
				shardStatusManager.syncShardList()
			case <-ctx.Done():
				ticker.Stop()
				return
			}
		}
	}()
}

func (shardStatusManager *ShardStatusManager) syncShardList() {
	shardStatusManager.Lock()
	shards := make(map[uint64]ShardNodeGroup)
	for nodeHash, node := range shardStatusManager.nodes {
		nodeBasicInfo, nodeMetricsInfo := shardStatusManager.shardMetricsManager.GetNodeShardInfo(node.GroupName, node.NodeId)
		nodeShardInfo := make(map[uint32]ShardStatus)
		for shardHash, info := range nodeMetricsInfo {
			shardInfo := ShardStatus{
				NodeId:          nodeBasicInfo.NodeId,
				GroupName:       nodeBasicInfo.GroupName,
				ShardId:         info.ShardId,
				Role:            info.Role,
				SeqNo:           info.SeqNo,
				BaseVersionHash: info.BaseVersionHash,
				ServiceState:    info.ServiceState,
				ShardHash:       shardHash,
			}
			nodeShardInfo[info.ShardId] = shardInfo
		}
		shards[nodeHash] = nodeShardInfo
	}
	shardStatusManager.Unlock()
	shardStatusManager.processShards(shards)
}

func (shardStatusManager *ShardStatusManager) processShards(shards map[uint64]ShardNodeGroup) {
	shardVector := make(map[uint32][]ShardStatus)
	shardCollections := make(map[string]ShardStatus)
	for _, shardList := range shards {
		for shardId, shardStatus := range shardList {
			shardVector[shardId] = append(shardVector[shardId], shardStatus)
			shardCollections[shardStatus.ShardHash] = shardStatus
		}
	}
	shardRelations := make(map[uint32]ShardRelation)
	for shardId, shardList := range shardVector {
		relationInfo := ShardRelation{
			Follower: make([]string, 0),
		}
		for _, shard := range shardList {
			if shard.Role == Leader {
				relationInfo.Leader = shard.ShardHash
			} else {
				relationInfo.Follower = append(relationInfo.Follower, shard.ShardHash)
			}
		}
		shardRelations[shardId] = relationInfo
	}

	// noDiff 计算
	for _, relation := range shardRelations {
		leaderShard, ok := shardCollections[relation.Leader]
		if !ok {
			for _, shardHash := range relation.Follower {
				shardStatus := shardCollections[shardHash]
				shardStatus.HasLeader = false
				shardCollections[shardHash] = shardStatus
			}
		} else {
			for _, shardHash := range relation.Follower {
				shardStatus := shardCollections[shardHash]

				shardStatus.NoDiff = false
				if shardStatus.BaseVersionHash != leaderShard.BaseVersionHash {
					shardStatus.NoDiff = true
				} else {
					diffValue := math.Abs(float64(shardStatus.SeqNo - leaderShard.SeqNo))
					diffRate := (diffValue / float64(shardStatus.SeqNo+1)) * 100
					if diffRate <= DefaultDiffRate || diffValue < DefaultDiffSeqNumbers {
						shardStatus.NoDiff = true
					}
				}
				shardStatus.HasLeader = true
				shardCollections[shardHash] = shardStatus
			}
		}
	}

	shardStatusManager.Lock()
	shardStatusManager.shardRelations = shardRelations
	shardStatusManager.shards = shardCollections
	shardStatusManager.Unlock()
}
