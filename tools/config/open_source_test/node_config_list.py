# -*- coding: UTF8 -*-
# vim: set expandtab tabstop=2 shiftwidth=2 softtabstop=2 foldmethod=marker: #
import json

def get_config() :
  config = {
    "NodeConfigList": {
      "default" : {
        "BlockCacheSizeGb" : 2,
        "HighPriPoolRatio" : 0.5,
        "StrictCapacityLimit" : True,
        "NumShardBits" : 6,
        "WriteBufferSizeGb" : 1
      }
    }
  }

  return json.dumps(config);
