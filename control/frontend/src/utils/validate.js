/**
 * Created by PanJiaChen on 16/11/18.
 */

/**
 * @param {string} path
 * @returns {Boolean}
 */
export function isExternal(path) { return /^(https?:|mailto:|tel:)/.test(path) }

/**
 * @param {string} str
 * @returns {Boolean}
 */
export function validUsername(str) {
  const valid_map = [ 'admin', 'editor' ];
  return valid_map.indexOf(str.trim()) >= 0;
}

export function validPartitionNumber(rule, value, callback) {
  if (value < 1) {
    return callback(new Error("分片数必须大于等于1"))
  }
  return callback()
}

export function validCommandLimitValue(rule, value, callback) {
  if (value < 0 || value > 100) {
    return callback(new Error("通过率须在0到100之间"))
  }
  return callback()
}

export function validEdgeFlowRatio(rule, value, callback) {
  if (value < 0 || value > 100) {
    return callback(new Error("边缘节点流量比例须在0到100之间"))
  }
  return callback()
}

export function validGroupReduceRate(rule, value, callback) {
  if (value < 0 || value > 100) {
    return callback(new Error("流量限制比例须在0到100之间"))
  }
  return callback()
}

export function validateIP(rule, value, callback) {
  let pattern = /^((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)$/g
  let valid = pattern.test(value)
  if (!valid) {
    return callback(new Error("IP地址无效"))
  }
  return callback()
}

export function validatePort(rule, value, callback) {
  if (value < 0 || value > 65535) {
    return callback(new Error("端口号无效"))
  }
  return callback()
}
