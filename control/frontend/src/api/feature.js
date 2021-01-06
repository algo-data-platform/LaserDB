import request from '@/utils/request'

export function getList(params) {
  return request({
    url: '/feature/list',
    method: 'get',
    params
  })
}

export function AddFeature(feature) {
  return request({
    url: '/feature/store',
    method: 'post',
    data: feature
  })
}

export function UpdateFeature(feature) {
  return request({
    url: '/feature/update',
    method: 'post',
    data: feature
  })
}
