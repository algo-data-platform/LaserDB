import request from '@/utils/request'

export function ListMachineCategory(params) { return request({ url: '/machine_category/list', method: 'get', params }) }

export function AddMachineCategory(category) { return request({ url: '/machine_category/store', method: 'post', data: category }) }

export function UpdateMachineCategory(category) { return request({ url: '/machine_category/update', method: 'post', data: category }) }

export function DeleteMachineCategory(params) { return request({ url: '/machine_category/delete', method: 'post', data: params }) }
