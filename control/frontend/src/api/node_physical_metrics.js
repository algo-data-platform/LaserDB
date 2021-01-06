import request from '@/utils/request'

export function ListNodePhysicalMetrics(params) {
  return request({
    url: '/node_physical_metric/list',
    method: 'get',
    timeout: 60000,
    params
  })
}

export function ListNodeList(params) {
  return request({
    url: '/node_physical_metric/node_list',
    method: 'get',
    params
  })
}
