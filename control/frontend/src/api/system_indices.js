import request from '@/utils/request'

export function ListSystemIndices(params) {
  return request({
    url: '/system_index/list',
    method: 'get',
    timeout: 60000,
    params
  })
}
