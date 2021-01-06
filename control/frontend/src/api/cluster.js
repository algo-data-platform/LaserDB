import request from '@/utils/request'

export function ListCluster(params) { return request({url : '/cluster/list', method : 'get', params}) }

export function AddCluster(cluster) { return request({url : '/cluster/store', method : 'post', data : cluster}) }

export function UpdateCluster(cluster) { return request({url : '/cluster/update', method : 'post', data : cluster}) }

export function SynchronizeClusterInfoToConsul() { return request({url : '/cluster/synchronize', method : 'post'}) }