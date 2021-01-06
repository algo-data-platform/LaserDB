import request from '@/utils/request'

export function getList(params) {
  return request({
    url: '/service/list',
    method: 'get',
    params
  })
}

export function AddService(service) {
  return request({
    url: '/service/store',
    method: 'post',
    data: service
  })
}

export function UpdateService(service) {
  return request({
    url: '/service/update',
    method: 'post',
    data: service
  })
}
