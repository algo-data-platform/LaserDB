import request from '@/utils/request'

export function ListTableRunningMetrics(params) {
  return request({
    url: '/table_metric/list',
    method: 'get',
    timeout: 60000,
    params
  })
}

export function ListTableList(params) {
  return request({
    url: '/table_metric/table_list',
    method: 'get',
    params
  })
}
