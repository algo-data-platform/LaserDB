import request from '@/utils/request'

export function ListNodeRunningMetrics(params) {
  return request({
    url: '/node_running_metric/list',
    method: 'get',
    timeout: 60000,
    params
  })
}

export function ListNodeList(params) {
  return request({
    url: '/node_running_metric/node_list',
    method: 'get',
    params
  })
}
