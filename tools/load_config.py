#!./venv/bin/python
# -*- coding: UTF8 -*-
# vim: set expandtab tabstop=2 shiftwidth=2 softtabstop=2 foldmethod=marker: #

import os, sys, json
import string
import requests
import pkgutil
import importlib
import gflags

# 所有配置的包
import config 

gflags.DEFINE_string('consul_address', '127.0.0.1:8500', 'consul server address')
gflags.DEFINE_string('env', 'dev', 'laser run env `dev/test/production`')
gflags.DEFINE_string('consul_token', '', 'consul server token')

# service router 的 service_manager load 功能未完成前使用该 mock 脚本开发
def load_json_file(dir_path) :
  data = {}
  parents = os.listdir(dir_path)
  for file_name in parents:
    file_path = os.path.join(dir_path, file_name)
    ext_info = os.path.splitext(file_name)
    if ext_info[1] != ".json" :
      continue
    with open(file_path, 'r') as f:
      data[ext_info[0]] = f.read()
  return data;

def create_kv(url, key, value) :
  path = url + '/v1/kv/' + key + '?token=' + gflags.FLAGS.consul_token
  headers = {}
  print(path)
  return requests.put(path, headers=headers, data=value)

def load_config_to_consul(url, service_name, config_module) :
  data = config_module.get_config()
  base_key = "ads-core/services/" + service_name + "/config/configs/"
  keys = config_module.__name__.split('.');
  if len(keys) != 3:
    print("config file is invalid.")
    return

  print(create_kv(url, base_key + keys[2] + "_data", data))

def load_config(url, env) :
  for _, env_pkg, _ in pkgutil.iter_modules(config.__path__, config.__name__ + "."):
    if env_pkg != config.__name__ + "." + env:
      continue
    package_env = importlib.import_module(env_pkg)
    for _, name, _ in pkgutil.iter_modules(package_env.__path__, package_env.__name__ + "."):
      service_name = "laser_" + env
      load_config_to_consul(url, service_name, importlib.import_module(name));

def main(argv):
  try:
    argv = gflags.FLAGS(argv)
  except gflags.FlagsError as e:
    print("%s\nUsage: %s ARGS\n%s" % (e, sys.argv[0], gflags.FLAGS))
    sys.exit(1)
  load_config("http://" + gflags.FLAGS.consul_address, gflags.FLAGS.env)

if __name__ == "__main__":
  main(sys.argv)
