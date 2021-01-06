#!./venv/bin/python
# -*- coding: UTF8 -*-
import os
import sys
import argparse
import requests
import traceback
import subprocess
import shutil

# 1. 获取 每个服务器中 数据表目录下的 partition 目录
# 2. 获取 每个服务器 线上有效的 partition (http 接口)
# 3. 找出那些本地有，但是线上没有的 partition, 但
# 4. 删除/移动 无用的 partitions


def get_local_node_databases():
  databases = []
  entries = os.listdir(args.node_dir)
  for entry in entries:
    if os.path.isdir(os.path.join(args.node_dir, entry)):
      databases.append(entry)
  print("Locale database: ", databases)
  return databases


def get_local_database_tables(database):
  tables = []
  database_dir = os.path.join(args.node_dir, database)
  entries = os.listdir(database_dir)
  for entry in entries:
    if os.path.isdir(os.path.join(database_dir, entry)):
      tables.append(entry)
  print("Tables of {0}: ".format(database), tables)
  return tables


def get_local_table_partrtion_list(database, table):
  partition_ids = []
  table_dir = os.path.join(args.node_dir, database, table)
  entries = os.listdir(table_dir)
  for entry in entries:
    if os.path.isdir(os.path.join(table_dir, entry)) and entry.isdigit:
      partition_ids.append(int(entry))
  print("Local partitions of {0}:{1}: ".format(database, table), partition_ids)
  return partition_ids


def get_online_table_partition_list(database, table):
  partition_ids = []
  url = 'http://{0}:{1}/db/meta?database_name={2}&table_name={3}'.format(
      args.host, args.http_port, database, table)
  response = requests.get(url)
  if response.status_code == 200:
    response_json = response.json()
    if response_json['Code'] == 0:
      partitions = response_json['Data']['Partitions']
      for partition in partitions:
        partition_ids.append(partition['PartitionId'])
  print("Online partitions of {0}:{1}: ".format(
      database, table), partition_ids)
  return partition_ids


def get_local_unused_partitions_to_delete(database, table):
  partitions_to_delete = []
  online_partition_ids = get_online_table_partition_list(database, table)
  local_partition_ids = get_local_table_partrtion_list(database, table)
  for id in local_partition_ids:
    if id not in online_partition_ids:
      partitions_to_delete.append(id)
  print("Partitions to delete of {0}:{1}: ".format(
      database, table), partitions_to_delete)
  return partitions_to_delete


def check_and_delete_unused_parition_dir(database, table, partition_id):
  partition_dir = os.path.join(
      args.node_dir, database, table, str(partition_id))
  # cmd = 'lsof -c laser_ | grep {0}'.format(partition_dir)
  # result = subprocess.Popen(
  #     ['/bin/sh', '-c', cmd], stdout=subprocess.PIPE)
  # output = result.communicate()
  # if output[0] == b'':
  #   # shutil.rmtree(partition_dir)
  if os.path.exists(partition_dir):
    dest_dir = os.path.join(args.dest_dir, database, table)
    if not os.path.exists(dest_dir):
      os.makedirs(dest_dir)
    # move_partition(partition_dir, dest_dir)
    shutil.rmtree(partition_dir)
    print('Delete partition:', partition_dir)


def clean_node_unused_partitions():
  databases = get_local_node_databases()
  for database in databases:
    tables = get_local_database_tables(database)
    for table in tables:
      unused_partitions_to_delete = get_local_unused_partitions_to_delete(
          database, table)
      for unused_partition in unused_partitions_to_delete:
        check_and_delete_unused_parition_dir(database, table, unused_partition)


def move_partition(partition_dir, dest_dir):
  shutil.move(partition_dir, dest_dir)


parser = argparse.ArgumentParser()
parser.add_argument('-host', dest='host', metavar='host ip',
                    required=True, action='store')
parser.add_argument('-port', dest='http_port', metavar='http port',
                    required=True, action='store')
parser.add_argument('-node-dir', dest='node_dir', metavar='node dir',
                    required=True, action='store')
parser.add_argument('-dest-dir', dest='dest_dir', metavar='dest dir',
                    required=True, action='store')
args = parser.parse_args()


def main(argv):
  try:
    if not os.path.exists(args.dest_dir):
      os.makedirs(args.dest_dir)
    clean_node_unused_partitions()
  except:
    traceback.print_exc()


if __name__ == '__main__':
  main(sys.argv)
