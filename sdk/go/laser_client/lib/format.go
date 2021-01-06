package lib

import (
	"github.com/zentures/cityhash"
)

type KeyType int

const (
	_ KeyType = iota
	KeyType_DEFAULT
	KeyType_COMPOSITE
	KeyType_TTL_SORT
)

type LaserKeyFormatBase struct {
	primaryKeys []string
	columns     []string
	keyType     KeyType
}

func NewLaserKeyFormatBase(
	primaryKeys,
	columns []string,
	keyType KeyType,
) *LaserKeyFormatBase {
	return &LaserKeyFormatBase{
		primaryKeys: primaryKeys,
		columns:     columns,
		keyType:     keyType,
	}
}

func (kfb *LaserKeyFormatBase) GetKeyHash() int64 {
	var res uint64 = 0
	for _, key := range kfb.primaryKeys {
		res = cityhash.CityHash64WithSeed([]byte(key), uint32(len(key)), res)
	}
	return int64(res)
}

func (kfb *LaserKeyFormatBase) GetPrimaryKeys() []string {
	return kfb.primaryKeys
}

func (kfb *LaserKeyFormatBase) GetColumns() []string {
	return kfb.columns
}

func (kfb *LaserKeyFormatBase) GetKeyType() KeyType {
	return kfb.keyType
}

type LaserKeyFormat struct {
	*LaserKeyFormatBase
}

func NewLaserKeyFormat(
	primaryKeys,
	columns []string,
) *LaserKeyFormat {
	return &LaserKeyFormat{
		LaserKeyFormatBase: NewLaserKeyFormatBase(primaryKeys, columns, KeyType_DEFAULT),
	}
}
