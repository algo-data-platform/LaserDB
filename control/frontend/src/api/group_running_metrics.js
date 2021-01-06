import request from '@/utils/request'

export function ListGroupRunningMetrics(params) {
  return request({
    url: '/group_running_metric/list',
    method: 'get',
    timeout: 60000,
    params
  })
}

export function ListGroupList(params) {
  return request({
    url: '/group_running_metric/group_list',
    method: 'get',
    params
  })
}
