import request from '@/utils/request'

export function checkGroupReadyToBeMaster(params) { return request({ url: '/config_validator/check_group_as_master', method: 'get', params }) }

export function checkUnsynchronizedConfig(params) { return request({ url: '/config_validator/unsynchronized_config', method: 'get' }) }
