package service

import "sync"

type ActionState uint32

const (
	AssignShardsState ActionState = iota + 1
	DisableDestinationNodeState
	PushToOnlineState
	WaitForUpdatingState
	UpdateShardsState
	RePushToOnlineState
)

type IShardControlActor interface {
	GetState()
	Run()
}

type ShardControlActor struct {
	sync.Mutex
	running      bool
	state        ActionState
	shardManager *ShardManager
}

func (actor *ShardControlActor) GetState() ActionState {
	return actor.state
}

func (actor *ShardControlActor) Cancel() {
	actor.Lock()
	defer actor.Unlock()
	actor.running = false
}

type FollowerDynamicAssignActor struct {
	ShardControlActor
}

func (actor *FollowerDynamicAssignActor) GetState() ActionState {
	return actor.GetState()
}

func (actor *FollowerDynamicAssignActor) AssignShards() {
	actor.shardManager.AssignShards()
}

func (actor *FollowerDynamicAssignActor) DisableDestinationNodeShards() {

}

func (actor *FollowerDynamicAssignActor) PushToOnline() {

}

func (actor *FollowerDynamicAssignActor) WaitForUpdating() {

}

func (actor *FollowerDynamicAssignActor) UpdateShards() {

}

func (actor *FollowerDynamicAssignActor) RePushToOnline() {

}
