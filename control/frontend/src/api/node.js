import request from '@/utils/request'

export function storeNode(node) { return request({ url: '/node/store', method: 'post', data: node }) }

export function batchStoreNode(nodes) { return request({ url: '/node/batch_store', method: 'post', data: nodes }) }

export function getList() { return request({ url: '/node/list', method: 'get' }) }

export function updateNode(node) { return request({ url: '/node/update', method: 'post', data: node }) }

export function changeNodeRole(data) { return request({ url: '/node/change_role', method: 'post', data: data }) }

export function deleteNode(node) { return request({ url: '/node/delete', method: 'post', data: node }) }

export function batchDeleteNodes(nodes) { return request({ url: '/node/batch_delete', method: 'post', data: nodes }) }

export function getShardList() { return request({ url: '/node/list_shards', method: 'get', timeout: 20000 }) }

export function ansibleOperation(params) { return request({ url: '/node/ansible', method: 'post', data: params }) }

export function getAnsibleOperationInfo(params) { return request({ url: '/node/operation_info', method: 'get', params }) }

