package service

import (
	"laser-control/context"
)

func InitService(ctx *context.Context) {
	ctx.Db().Set("gorm:table_options", "ENGINE=InnoDB CHARSET=utf8")
	ctx.Db().AutoMigrate(&LaserDatabase{})
	ctx.Db().AutoMigrate(&Table{})
	ctx.Db().AutoMigrate(&ShardStore{})
	ctx.Db().AutoMigrate(&TableConfigItem{})
	ctx.Db().AutoMigrate(&TableConfig{})
	ctx.Db().AutoMigrate(&TrafficRestrictionLimit{})
	ctx.Db().AutoMigrate(&ProxyTableConfig{})
	ctx.Db().AutoMigrate(&NodeRateLimitEntry{})
	ctx.Db().AutoMigrate(&NodeConfig{})
	ctx.Db().AutoMigrate(&Cluster{})
	ctx.Db().AutoMigrate(&Group{})
	ctx.Db().AutoMigrate(&Node{})
	ctx.Db().AutoMigrate(&MachineCategory{})
	ctx.Db().AutoMigrate(&Machine{})
	ctx.Db().AutoMigrate(&AnsibleConfig{})
	ctx.Db().AutoMigrate(&Ticket{})
	ctx.Db().AutoMigrate(&Dc{})
	ctx.Db().AutoMigrate(&NodePhysicalMetrics{})
	ctx.Db().AutoMigrate(&NodeRunningMetrics{})
	ctx.Db().AutoMigrate(&GroupRunningMetrics{})
	ctx.Db().AutoMigrate(&ResourcePoolRunningMetrics{})
	ctx.Db().AutoMigrate(&ClusterRunningMetrics{})
	ctx.Db().AutoMigrate(&TableMetrics{})
	ctx.Db().AutoMigrate(&SystemIndex{})

	// 初始化 shard node 管理器
	InitShard(ctx)
	GetReportDataCollector(ctx)
}

func InitShard(ctx *context.Context) {
	node := GetNodeModel(ctx)
	GetShardModel(ctx, node.cluster)
}
