
import request from '@/utils/request'

export function AssignTableShardList(params) {
  return request({ url: '/tools/assign_table_shard_list', method: 'get', timeout: 100000, params })
}
