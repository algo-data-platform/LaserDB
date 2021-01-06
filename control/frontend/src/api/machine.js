import request from '@/utils/request'

export function ListMachine(params) { return request({url : '/machine/list', method : 'get', params}) }

export function AddMachine(machine) { return request({url : '/machine/store', method : 'post', data : machine}) }

export function UpdateMachine(machine) { return request({url : '/machine/update', method : 'post', data : machine}) }

export function DeleteMachine(machine) { return request({url : '/machine/delete', method : 'post', data : machine}) }
