import request from '@/utils/request'

export function ListPoolRunningMetrics(params) {
  return request({
    url: '/resource_pool_running_metric/list',
    method: 'get',
    timeout: 60000,
    params
  })
}

export function ListPoolList(params) {
  return request({
    url: '/resource_pool_running_metric/pool_list',
    method: 'get',
    params
  })
}
