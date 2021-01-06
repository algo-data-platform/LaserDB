import request from '@/utils/request'

export function ListTable(params) {
  return request({
    url : '/table/list',
    method : 'get', params
  })
}

export function ListTableDetail(params) {
  return request({
    url : '/table/list_detail',
    method : 'get', params
  })
}

export function AddTable(table) {
  return request({
    url: '/table/store',
    method: 'post',
    data: table
  })
}

export function UpdateTable(table) {
  return request({
    url : '/table/update',
    method : 'post',
    data : table
  })
}

export function DeleteTable(table) {
  return request({
    url : '/table/delete',
    method : 'post',
    data : table
  })
}

export function SynchronizeDataToConsul() {
  return request({
    url : 'table/synchronize',
    method : 'post'
  })
}

export function ListCommands() {
  return request({
    url : '/table/list_commands',
    method : 'get'
  })
}
