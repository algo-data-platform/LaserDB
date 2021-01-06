import request from '@/utils/request'

export function ListAnsibleConfig(params) {
  return request({
    url: '/ansible_config/list',
    method: 'get',
    params
  })
}

export function AddAnsibleConfig(table) {
  return request({
    url: '/ansible_config/store',
    method: 'post',
    data: table
  })
}

export function UpdateAnsibleConfig(table) {
  return request({
    url: '/ansible_config/update',
    method: 'post',
    data: table
  })
}

export function DeleteAnsibleConfig(table) {
  return request({
    url: '/ansible_config/delete',
    method: 'post',
    data: table
  })
}
