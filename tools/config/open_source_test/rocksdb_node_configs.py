# -*- coding: UTF8 -*-
# vim: set expandtab tabstop=2 shiftwidth=2 softtabstop=2 foldmethod=marker: #
import json

def get_config() :
  config = {
    "RocksdbNodeConfigs": {
      "group-a#1": "default",
      "group-a#2": "default",
    }
  }

  return json.dumps(config);
