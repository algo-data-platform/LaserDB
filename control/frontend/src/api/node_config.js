import request from '@/utils/request'

export function NodeListConfig(params) {
  return request({
    url: '/node_config/list',
    method: 'get',
    params
  })
}

export function NodeAddConfig(node_config) {
  return request({
    url: '/node_config/store',
    method: 'post',
    data: node_config
  })
}

export function NodeUpdateConfig(node_config) {
  return request({
    url: '/node_config/update',
    method: 'post',
    data: node_config
  })
}

export function DeleteNodeConfig(node_config) {
  return request({
    url: '/node_config/delete',
    method: 'post',
    data: node_config
  })
}
