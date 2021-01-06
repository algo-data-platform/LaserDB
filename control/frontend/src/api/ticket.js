import request from '@/utils/request'

export function ListTicket(params) { return request({url : '/ticket/list', method : 'get', params}) }

export function AddTicket(ticket) { return request({url : '/ticket/store', method : 'post', data : ticket}) }

export function UpdateTicket(ticket) { return request({url : '/ticket/update', method : 'post', data : ticket}) }

export function ProcessTicket(ticket) { return request({url : '/ticket/process', method : 'post', data : ticket}) }

export function DeleteTicket(ticket) { return request({url : '/ticket/delete', method : 'post', data : ticket}) }
