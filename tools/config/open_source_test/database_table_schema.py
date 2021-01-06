# -*- coding: UTF8 -*-
# vim: set expandtab tabstop=2 shiftwidth=2 softtabstop=2 foldmethod=marker: #
import json

# 本地 db 规则：
# 
# /data/group_name/node_id/database_name/table_name/partition_id/
# /source_data/base/database_name/table_name/partition_id/version
# /source_data/delta/database_name/table_name/partition_id/version
#
# hdfs 存储定义规则：
# 
#   a. base 方式： hdfs://ad-core/base/database_name/table_name/partition_id/[version]
#   b. delta 方式： hdfs://ad-core/delta/database_name/table_name/partition_id/[version]
#
# base 和 delta 的 sst 文件生成方式 
#   
#   通过 sst 转化脚本将数据源文件转化为 sst 文件
# 
#   数据源文件格式如下：
#   {
#       "primary_keys": [
#           "uid",
#           "page_id"
#       ],
#       "columns": [
#           "name",
#           "age"
#       ],
#       "value_type": "map",
#       "data": {
#           "k1": "v1",
#           "k2": "v2"
#       }
#   }
#
# base 和 delta 的 sst 文件加载方式
#
#   base: 将 sst 导入到一个新的 db 中，等新的 db 完全的重建完成后直接切换到新的上面，旧的db 直接回收删除即可
#   delta: 将 sst 导入到一个新的 db 中，遍历所有的 key 将所有key 进行patch 到 base db 上

def get_config() :
  config = [
    {
      "DatabaseName" : "test",
      "Tables" : [ 
        {
          "TableName" : "test",
          "PartitionNumber": 10,
          "ConfigName": "default",
        },
      ]
    }
  ]

  return json.dumps(config);
