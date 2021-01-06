export function newNodeInfo() {
  return {
    Id: 0,
    NodeId: undefined,
    Active: 0,
    Status: 1,
    Host: '',
    Port: undefined,
    Weight: 0,
    Master: false,
    IsEdgeNode: false,
    Desc: '',
    GroupId: 0,
    GroupName: '',
    ConfigId: 0,
    ConfigName: '',
    AnsibleConfigId: undefined,
    AnsibleConfigName: '',
    LeaderShardList: '',
    FollowerShardList: ''
  }
}

export function string24HourToNumber(stringHour) {
  let [hours, minutes] = stringHour.split(':');
  return parseInt(hours)
}

export function number24HourToString(numberHour) {
  const hours = numberHour.toString().padStart(2, '0')
  return hours + ':' + '00'
}

export function checkForm(component, formName) {
  let _self = component
  let result = new Promise(function (resolve, reject) {
    let form = _self.$refs[formName]
    if (form != undefined) {
      form.validate(valid => {
        if (valid) {
          resolve()
        } else {
          reject()
        }
      })
    } else {
      resolve()
    }
  })
  return result
}

export function clearFormValidate(component, formName) {
  component.$refs[formName].clearValidate()
}

export function getNDaysBefore(n) {
  const MS_PER_DAY = 1000 * 3600 * 24
  let today = new Date()
  let nDaysBeforeInMs = today.getTime() - n * MS_PER_DAY
  let nDaysBefore = new Date()
  nDaysBefore.setTime(nDaysBeforeInMs)
  return nDaysBefore
}

Date.prototype.format = function (fmt) {
  var o = { 
    "M+" : this.getMonth()+1,                 //月份 
    "d+" : this.getDate(),                    //日 
    "h+" : this.getHours(),                   //小时 
    "m+" : this.getMinutes(),                 //分 
    "s+" : this.getSeconds(),                 //秒 
    "q+" : Math.floor((this.getMonth()+3)/3), //季度 
    "S"  : this.getMilliseconds()             //毫秒 
  }; 
  if(/(y+)/.test(fmt)) {
          fmt=fmt.replace(RegExp.$1, (this.getFullYear()+"").substr(4 - RegExp.$1.length)); 
  }
  for(var k in o) {
      if(new RegExp("("+ k +")").test(fmt)){
          fmt = fmt.replace(RegExp.$1, (RegExp.$1.length==1) ? (o[k]) : (("00"+ o[k]).substr((""+ o[k]).length)));
      }
  }
  return fmt;
}
