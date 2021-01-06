import request from '@/utils/request'

export function getList(params) {
  return request({
    url: '/view/list',
    method: 'get',
    params
  })
}

export function getInfo(id) {
  return request({
    url: '/view/info/' + id,
    method: 'get'
  })
}

export function AddView(view) {
  return request({
    url: '/view/store',
    method: 'post',
    data: view
  })
}

export function UpdateView(view) {
  return request({
    url: '/view/update',
    method: 'post',
    data: view
  })
}
