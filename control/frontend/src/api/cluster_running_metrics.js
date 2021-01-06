import request from '@/utils/request'

export function ListClusterRunningMetrics(params) {
  return request({
    url: '/cluster_running_metric/list',
    method: 'get',
    timeout: 60000,
    params
  })
}
