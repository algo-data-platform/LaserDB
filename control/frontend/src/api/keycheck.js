import request from '@/utils/request'

export function ListKeycheck(params) {
  return request({
    url: '/keycheck/list',
    method: 'post',
    data: params
  })
}
