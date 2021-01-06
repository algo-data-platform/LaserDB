import request from '@/utils/request'

export function VersionChangeShow(params) {
  return request({
    url: 'version_change/show',
    method: 'get',
    params
  })
}

export function VersionChangeClear(db) {
  return request({
    url: 'version_change/clear',
    method: 'post',
    data: db
  })
}

export function VersionChangeVersion(db) {
  return request({
    url: 'version_change/version',
    method: 'post',
    data: db
  })
}

export function VersionChangeRollback(db) {
  return request({
    url: 'version_change/rollback',
    method: 'post',
    data: db
  })
}