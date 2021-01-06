import request from '@/utils/request'

export function ListDatabase(params) {
  return request({
    url: '/database/list',
    method: 'get',
    params
  })
}

export function AddDatabase(db) {
  return request({
    url: '/database/store',
    method: 'post',
    data: db
  })
}

export function UpdateDatabase(db) {
  return request({
    url: '/database/update',
    method: 'post',
    data: db
  })
}

export function DeleteDatabase(db) {
  return request({
    url: '/database/delete',
    method: 'post',
    data: db
  })
}
