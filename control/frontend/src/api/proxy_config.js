import request from '@/utils/request'

export function ProxyListTableConfig(params) {
  return request({
    url: '/proxy_config/list',
    method: 'get',
    params
  })
}

export function ProxyAddTableConfig(proxy_table_config) {
  return request({
    url: '/proxy_config/store',
    method: 'post',
    data: proxy_table_config
  })
}

export function ProxyUpdateTableConfig(proxy_table_config) {
  return request({
    url: '/proxy_config/update',
    method: 'post',
    data: proxy_table_config
  })
}

export function ProxySynchronizeDataToConsul() {
  return request({
    url: 'proxy_config/synchronize',
    method: 'post'
  })
}

