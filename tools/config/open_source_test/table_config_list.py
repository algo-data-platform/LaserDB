# -*- coding: UTF8 -*-
# vim: set expandtab tabstop=2 shiftwidth=2 softtabstop=2 foldmethod=marker: #
import json

def get_config() :
  config = {
    "TableConfigList": {
      "default":{
        "DbOptions" : {
          "create_if_missing": "true",
          "max_background_flushes": "20", 
          "max_background_compactions": "20",
          "WAL_ttl_seconds" : "3600",
          "WAL_size_limit_MB" : "512",
          "use_direct_reads": "true",
          "compaction_readahead_size": "2097152",
        },
        "CfOptions" : {
          "num_levels": "7",
        },
        "TableOptions" : {
          "filter_policy": "bloomfilter:10:false",
          "index_type": "kTwoLevelIndexSearch",
          "data_block_index_type": "kDataBlockBinaryAndHash",
          "partition_filters": "true",
          "metadata_block_size": "4096",
          "cache_index_and_filter_blocks_with_high_priority": "true",
          "cache_index_and_filter_blocks": "true",
          "pin_l0_filter_and_index_blocks_in_cache": "true",
          "block_size": "65536"
        },
        "Version": 1
      },
    }
  }

  return json.dumps(config);
