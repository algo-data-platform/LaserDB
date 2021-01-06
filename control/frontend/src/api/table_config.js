import request from '@/utils/request'

export function ListTableConfig(params) {
  return request({
    url: '/table_config/list',
    method: 'get',
    params
  })
}

export function ListTableConfigWithDetailItems(params) {
  return request({
    url: '/table_config/list_detail',
    method: 'get',
    params
  })
}

export function AddTableConfig(table) {
  return request({
    url: '/table_config/store',
    method: 'post',
    data: table
  })
}

export function UpdateTableConfig(table) {
  return request({
    url: '/table_config/update',
    method: 'post',
    data: table
  })
}

export function DeleteTableConfig(table) {
  return request({
    url: '/table_config/delete',
    method: 'post',
    data: table
  })
}
