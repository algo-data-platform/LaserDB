package lib

import (
	"github.com/zentures/cityhash"
)

const UINT32MAX int64 = 4294967295 // 2^32 - 1

func abs(v int64) int64 {
	if v < 0 {
		return -1 * v
	}
	return v
}

func GetPartitionId(dbName, tableName string, formatKey *LaserKeyFormat, partitionNumber int) uint32 {
	key := cityhash.CityHash64WithSeed([]byte(dbName), uint32(len(dbName)), uint64(formatKey.GetKeyHash()))
	key = cityhash.CityHash64WithSeed([]byte(tableName), uint32(len(tableName)), key)
	return uint32(abs(int64(key)) % int64(partitionNumber))
}

func getShardId(partitionHash int64, shardNumber uint32) int64 {
	if shardNumber == 0 {
		return UINT32MAX
	}
	return abs(partitionHash) % int64(shardNumber)
}

func GetShardId(partition *Partition, config *ConfigManager, dc string) int64 {
	return getShardId(partition.GetPartitionHash(), config.GetShardNumber(dc))
}

func GetShardIdByShardNumber(partition *Partition, shardNumber uint32) int64 {
	return getShardId(partition.GetPartitionHash(), shardNumber)
}

type DbRole string

const (
	DbRole_LEADER   DbRole = "leader"
	DbRole_Follower DbRole = "follower"
)

type Partition struct {
	databaseName string
	tableName    string
	partitionId  uint32
	shardId      int64
	dbRole       DbRole
}

func (p *Partition) GetPartitionHash() int64 {
	key := cityhash.CityHash64WithSeed([]byte(p.databaseName), uint32(len(p.databaseName)), uint64(p.partitionId))
	key = cityhash.CityHash64WithSeed([]byte(p.tableName), uint32(len(p.tableName)), key)
	return int64(key)
}

func NewPartition(dbName, tableName string, partitionId uint32) *Partition {
	return &Partition{
		databaseName: dbName,
		tableName:    tableName,
		partitionId:  partitionId,
		shardId:      0,
		dbRole:       DbRole_LEADER,
	}
}

type PartitionManager struct {
}
