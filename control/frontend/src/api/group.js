import request from '@/utils/request'

export function ListGroup(params) { return request({url : '/group/list', method : 'get', params}) }

export function AddGroup(group) { return request({url : '/group/store', method : 'post', data : group}) }

export function UpdateGroup(group) { return request({url : '/group/update', method : 'post', data : group}) }

export function ReduceMetricsGroup(group) { return request({url : '/group/reduce_metrics', method : 'post', data : group}) }

export function DeleteGroup(group) { return request({url : '/group/delete', method : 'post', data : group}) }
