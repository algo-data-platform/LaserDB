# -*- coding: UTF8 -*-
# vim: set expandtab tabstop=2 shiftwidth=2 softtabstop=2 foldmethod=marker: #
import json

#
# 通过调度器脚本生成
#
# 通过 service_manager 脚本同步到 consul 上去
# 
# Node 处理流程：
# 
# 1. 探测到配置更新解析当前配置，查找对应 group -> nodeid 的节点信息，可本地的配置进行对比
# 2. 对比结果处理逻辑：
#    a. 没更新，不做任何处理
#    b. shard id 有增加，则需要暂停服务，进行load shard 对应的所有表的分区，同时如果有减少的 shard 则删除掉
#    c. 仅仅是扩容时减少 shard 则无需暂停服务，只是更新 service shard list 即可
#
#  NB: shard 变更仅允许单个 group 变更，不然可能会影响线上服务
#
# key -> partition -> shard 映射规则
#   partition_id = hash(key) % part_number
#   shard_id = hash(database_name + table_name + partid) % shard_number;
#
# shard_id -> partition 查找方式
#
# 获取所有 db table 配置进行枚举：
# class TablePartition
#   string database_name
#   string table_name
#   string partition_id
#
# std::unordered_map<int, std::vector<TablePartition>> shard_to_partition_map;
# for (auto db :  databases) {
#   for (auto table :  db->getTables()) {
#     for (int i = 0; i < table->getPartitionNumber()) {
#       int shard_id = hash(database_name + table_name + i) % shard_number;
#       auto list = shard_to_partition_map[shard_id];
#       TablePartition partition_info;
#       partition_info.database_name = db->getDatabaseName();
#       partition_info.table_name = table->getTableName();
#       partition_info.partition_id = i;
#       list.push_back(partition_info);
#     }
#   }
# }
# 将映射关系缓存起来，和 database 配置文件联动变化，当shard 变化时直接获取对应的表和 partition 信息进行对应操作
# 此处不需要有 host:ip 信息，具体的 node_id 和 host::port 在实例启动时指定的 flags
#

def get_config() :
  config = {
    "ClusterName" : "open_source_test",
    "ShardNumber" : 10,
    "Groups" : [
      {
        "GroupName" : "group-a",
        "Nodes" : [
          {
            "NodeId" : 1,
            "LeaderShardList": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
            "FollowerShardList": [],
            "IsEdgeNode": False,
          },
          {
            "NodeId" : 2,
            "LeaderShardList": [],
            "FollowerShardList": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
            "IsEdgeNode": False,
          },
        ]
      },
    ]
  }

  return json.dumps(config);
