<template>
  <div style="margin-left:10px">
    <el-row :gutter="20">
      <el-button type="success" icon="el-icon-video-play" @click="enable"
        :disabled="shouldBeDisabled(operations.enable.Name)" :loading="shouldBeLoading(operations.enable.Name)">启用
      </el-button>
      <el-button type="warning" icon="el-icon-video-pause" @click="disable"
        :disabled="shouldBeDisabled(operations.disable.Name)" :loading="shouldBeLoading(operations.disable.Name)">停用
      </el-button>
      <el-button type="success" icon="el-icon-s-promotion" @click="start"
        :disabled="shouldBeDisabled(operations.start.Name)" :loading="shouldBeLoading(operations.start.Name)">启动
      </el-button>
      <el-button type="danger" icon="el-icon-switch-button" @click="stop"
        :disabled="shouldBeDisabled(operations.stop.Name)" :loading="shouldBeLoading(operations.stop.Name)">停止
      </el-button>
      <el-button type="warning" icon="el-icon-refresh-right" @click="restart"
        :disabled="shouldBeDisabled(operations.restart.Name)" :loading="shouldBeLoading(operations.restart.Name)">重启
      </el-button>
      <el-button type="danger" icon="el-icon-delete" @click="batchDelete"
        :disabled="shouldBeDisabled(operations.batchDelete.Name)"
        :loading="shouldBeLoading(operations.batchDelete.Name)">删除
      </el-button>
      <el-button type="warning" icon="el-icon-refresh" @click="upgrade"
        :disabled="shouldBeDisabled(operations.upgrade.Name)" :loading="shouldBeLoading(operations.upgrade.Name)">更新
      </el-button>
      <el-button type="success" icon="el-icon-monitor" @click="handleConsole">控制台</el-button>
      <el-button type="primary" icon="el-icon-circle-plus" @click="$emit('handleBatchCreate')"
        :disabled="shouldBeDisabled(operations.batchAdd.Name)" :loading="shouldBeLoading(operations.batchAdd.Name)">
        批量添加边缘节点
      </el-button>
      <el-dropdown @command="command=>changeRole(command)">
        <el-button type="warning" :disabled="shouldBeDisabled(operations.changeRole.Name)"
          :loading="shouldBeLoading(operations.changeRole.Name)">
          角色设置<i class="el-icon-arrow-down el-icon--right"></i>
        </el-button>
        <el-dropdown-menu slot="dropdown">
          <el-dropdown-item :command="roles.Master">当前组设为主副本</el-dropdown-item>
          <el-dropdown-item :command="roles.Follower">当前组设为从副本</el-dropdown-item>
        </el-dropdown-menu>
      </el-dropdown>
      <el-checkbox class="filter-item" style="margin-left:15px;"
        @change="checked=>$emit('handleEnableBatchOperation',checked)">
        批量操作
      </el-checkbox>
      <el-tag size="small" v-if="nodesToOperateOn.size !== 0">选中 {{ nodesToOperateOn.size }} 项</el-tag>
    </el-row>

    <el-dialog ref="consoleDialog" title="控制台" :visible.sync="consoleDialogVisible" width="100%">
      <div class="console" v-html="html">
      </div>
    </el-dialog>
  </div>
</template>

<style lang="scss" scoped>
.console {
  font-family: monospace;
  text-align: left;
  background-color: black;
  color: #fff;
  overflow-y: auto;
}
</style>

<script>
import AnsiUp from 'ansi_up'
import { Message, Loading } from 'element-ui'
import {
  batchDeleteNode,
  ansibleOperation,
  getAnsibleOperationInfo,
  changeNodeRole
} from '@/api/node'
import { ListAnsibleConfig } from '@/api/ansible_config'
import { checkGroupReadyToBeMaster } from '@/api/config_validator'

export default {
  name: 'AnsibleOperation',
  props: {
    nodesToOperateOn: {
      type: Map,
      default() {
        return new Map()
      }
    },
    groupName: {
      type: String,
      default: ''
    },
    groupId: {
      type: Number
    },
    operations: {
      type: Object,
      default() {
        return {}
      }
    }
  },
  data() {
    return {
      ansibleOperationParams: {
        Nodes: [],
        AnsibleVars: {},
        Tags: '',
        Roles: [],
        OperationName: ''
      },
      userAbortMessage: 'user abort',
      ansi: undefined,
      qureyOperationInfoParams: {
        GroupName: this.groupName
      },
      timeoutId: undefined,
      consoleContent: '',
      consoleDialogVisible: false,
      consoleDialogLoading: undefined,
      operationInProgress: false,
      currentOperationName: '',
      queryAnsibleConfigParams: {
        Page: undefined,
        Limit: undefined,
        Id: undefined
      },
      roles: {
        Master: 'master',
        Follower: 'follower'
      },
      changeRoleParams: {
        Id: undefined,
        GroupId: this.groupId,
        Master: false
      }
    }
  },
  computed: {
    html() {
      return this.ansi.ansi_to_html(this.consoleContent).replace(/\n/gm, '<br>')
    },
    showConsoleDialogLoading() {
      return this.operationInProgress && this.consoleContent.length === 0
    }
  },
  watch: {
    showConsoleDialogLoading(val) {
      if (val === true) {
        this.createConsoleDialogLoading()
      } else {
        this.consoleDialogLoading.close()
      }
    }
  },
  beforeMount() {
    this.ansi = new AnsiUp()
  },
  created() {
    this.getOperationInfo()
  },
  updated() {
    // auto-scroll to the bottom when the DOM is updated
    this.$el.scrollTop = this.$el.scrollHeight
  },
  methods: {
    shouldBeDisabled(operationName) {
      if (
        operationName !== this.currentOperationName &&
        this.operationInProgress === true
      ) {
        return true
      }
      return false
    },
    shouldBeLoading(operationName) {
      if (
        operationName === this.currentOperationName &&
        this.operationInProgress === true
      ) {
        return true
      }
      return false
    },
    enable() {
      this.ansibleOperation(this.operations.enable)
    },
    disable() {
      this.ansibleOperation(this.operations.disable)
    },
    start() {
      this.ansibleOperation(this.operations.start)
    },
    stop() {
      this.checkNodeCanStop()
        .then(() => {
          this.ansibleOperation(this.operations.stop)
        })
        .catch(e => {
          if (e.message !== this.userAbortMessage) {
            throw e
          }
        })
    },
    restart() {
      this.checkNodeCanStop()
        .then(() => {
          this.ansibleOperation(this.operations.restart)
        })
        .catch(e => {
          if (e.message !== this.userAbortMessage) {
            throw e
          }
        })
    },
    upgrade() {
      this.checkNodeCanStop()
        .then(() => {
          this.ansibleOperation(this.operations.upgrade)
        })
        .catch(e => {
          if (e.message !== this.userAbortMessage) {
            throw e
          }
        })
    },
    batchDelete() {
      if (!this.checkParams()) {
        return
      }
      this.$emit('batchDeleteNodes')
    },
    changeRole(command) {
      this.changeRoleParams.Master = false
      if (this.roles.Master === command) {
        this.changeRoleParams.Master = true
      }
      this.checkGroupReadyToBeMaster(this.changeRoleParams.Master)
        .then(() => {
          this.operationInProgress = true
          this.currentOperationName = this.operations.changeRole.Name
          changeNodeRole(this.changeRoleParams)
            .then(response => {
              this.$notify({
                title: 'Success',
                message: 'Role change Successfully',
                type: 'success',
                duration: 2000
              })
            })
            .finally(() => {
              this.operationInProgress = false
            })
        })
        .catch(e => {
          if (e.message !== this.userAbortMessage) {
            throw e
          }
        })
    },
    checkParams() {
      if (this.nodesToOperateOn.size === 0) {
        Message({
          showClose: true,
          message: '当前没有选中的节点，请选择要操作的节点',
          type: 'warning',
          center: true,
          duration: 2000
        })
        return false
      }
      return true
    },
    checkNodeCanStop() {
      const self = this
      return new Promise(function(resolve, reject) {
        let warningText =
          '<p><strong>以下节点 KPS 不为0，仍要继续操作？</strong></p>'
        let noneZeroKpsNum = 0
        self.nodesToOperateOn.forEach((value, key, map) => {
          if (value.ReadKps !== 0 || value.WriteKps !== 0) {
            const hostInfo = value.Host + ':' + value.Port
            const kpsInfo =
              'WriteKps: ' + value.WriteKps + ' ReadKps: ' + value.ReadKps
            warningText += '<p>' + hostInfo + '--' + kpsInfo + '</p>'
            ++noneZeroKpsNum
          }
        })

        if (noneZeroKpsNum !== 0) {
          self
            .$confirm(warningText, '提示', {
              confirmButtonText: '确定',
              cancelButtonText: '取消',
              dangerouslyUseHTMLString: true,
              type: 'warning'
            })
            .then(() => {
              resolve()
            })
            .catch(() => {
              reject(new Error(self.userAbortMessage))
              self.$emit('postAnsibleOperation')
            })
        } else {
          resolve()
        }
      })
    },
    checkAnsibleConfigs() {
      const self = this
      return new Promise(function(resolve, reject) {
        let nodeAnsibleConfigText = ''
        let ansibleConfigIdSet = new Set()
        let nodeHasNoAnsibleConfig = false
        self.nodesToOperateOn.forEach((value, key, map) => {
          if (value.AnsibleConfigId !== 0) {
            ansibleConfigIdSet.add(value.AnsibleConfigId)
          } else {
            nodeHasNoAnsibleConfig = true
          }
          const hostInfo = value.Host + ':' + value.Port
          nodeAnsibleConfigText +=
            '<p>' + hostInfo + '--' + value.AnsibleConfigName + '</p>'
        })

        let showWarning = false
        let warningText = ''
        if (ansibleConfigIdSet.size === 1 && nodeHasNoAnsibleConfig === false) {
          self.queryAnsibleConfigParams.Id = ansibleConfigIdSet
            .values()
            .next().value
          resolve()
        } else if (
          ansibleConfigIdSet.size === 0 ||
          nodeHasNoAnsibleConfig === true
        ) {
          showWarning = true
          warningText =
            '<p><strong>选择的节点没有设置启动参数，请为节点设置启动参数！</strong></p>'
        } else if (ansibleConfigIdSet.size > 1) {
          showWarning = true
          warningText =
            '<p><strong>选择的节点启动参数不一致，不可以同时执行操作！</strong></p>'
        }
        if (showWarning === true) {
          warningText += nodeAnsibleConfigText
          self
            .$alert(warningText, '提示', {
              confirmButtonText: '确定',
              dangerouslyUseHTMLString: true,
              type: 'warning'
            })
            .then(() => {
              reject(new Error(self.userAbortMessage))
              self.$emit('postAnsibleOperation')
            })
        }
      })
    },
    checkGroupReadyToBeMaster(changeToMaster) {
      if (changeToMaster === true) {
        const self = this
        return new Promise(function(resolve, reject) {
          const param = {
            GroupName: self.groupName
          }
          checkGroupReadyToBeMaster(param).then(response => {
            const data = response.Data
            if (data.Ready === true) {
              resolve()
            } else {
              const numSortFunc = function(num1, num2) {
                return num1 - num2
              }
              let warningText =
                '<p><strong>当前组无法切换为主副本！</strong></p>'
              if (data.MissingShards.length !== 0) {
                data.MissingShards = data.MissingShards.sort(numSortFunc)
                warningText +=
                  '<p><strong> 缺少分片：</strong>' +
                  String(data.MissingShards) +
                  '</p>'
              }
              if (data.ReduplicativeShards.length !== 0) {
                data.ReduplicativeShards = data.ReduplicativeShards.sort(
                  numSortFunc
                )
                warningText +=
                  '<p><strong> 重复分片：</strong>' +
                  String(data.ReduplicativeShards) +
                  '</p>'
              }
              if (Object.keys(data.UnavailableShards).length !== 0) {
                warningText += '<p><strong> Unavailable 分片：</strong></p>'
                for (let nodeId in data.UnavailableShards) {
                  const shards = data.UnavailableShards[nodeId].sort(
                    numSortFunc
                  )
                  warningText +=
                    '<p> NodeId: ' +
                    nodeId +
                    ' Shards: ' +
                    String(shards) +
                    '</p>'
                }
              }

              if (data.InconsistentNodes.length !== 0) {
                const nodes = data.InconsistentNodes.sort(numSortFunc)
                warningText +=
                  '<p><strong> 节点分片列表与 Server 不一致, 节点 ID：</strong>' +
                  String(nodes) +
                  '</p>'
              }

              self
                .$alert(warningText, '提示', {
                  confirmButtonText: '确定',
                  dangerouslyUseHTMLString: true,
                  type: 'warning'
                })
                .then(() => {
                  reject(new Error(self.userAbortMessage))
                })
            }
          })
        })
      } else {
        return new Promise(function(resolve, reject) {
          resolve()
        })
      }
    },
    ansibleOperation(operation) {
      if (!this.checkParams()) {
        this.$emit('postAnsibleOperation')
        return
      }
      this.checkAnsibleConfigs()
        .then(() => {
          this.consoleDialogVisible = true
          this.operationInProgress = true
          this.currentOperationName = operation.Name
          this.ansibleOperationParams.OperationName = operation.Name
          this.ansibleOperationParams.Tags = operation.Tags
          this.ansibleOperationParams.Nodes = []
          this.nodesToOperateOn.forEach((value, key, map) => {
            const nodeInfo = {
              Host: value.Host,
              Port: value.Port,
              NodeId: value.NodeId,
              GroupName: value.GroupName,
              Dc: value.Dc,
            }
            this.ansibleOperationParams.Nodes.push(nodeInfo)
          })
          ListAnsibleConfig(this.queryAnsibleConfigParams).then(response => {
            const roles = response.Data.Items[0].Roles
            const vars = response.Data.Items[0].Vars
            this.ansibleOperationParams.Roles = []
            roles.split(',').forEach(role => {
              this.ansibleOperationParams.Roles.push(role.trim())
            })
            const varArray = JSON.parse(vars)
            let varObject = {}
            varArray.forEach(ansbileVar => {
              varObject[ansbileVar.Name] = ansbileVar.Value
            })
            this.ansibleOperationParams.AnsibleVars = varObject
            ansibleOperation(this.ansibleOperationParams)
              .then(response => {
                this.getOperationInfo()
              })
              .catch(e => {
                this.operationInProgress = false
                throw e
              })
              .finally(() => {
                this.$emit('postAnsibleOperation')
              })
          })
        })
        .catch(e => {
          if (e.message !== this.userAbortMessage) {
            throw e
          }
        })
    },
    getOperationInfo() {
      getAnsibleOperationInfo(this.qureyOperationInfoParams)
        .then(response => {
          this.consoleContent = response.Data.Log
          if (response.Data.HasTask === true) {
            if (response.Data.Done === true) {
              this.operationInProgress = false
              clearTimeout(this.timeoutId)
            } else if (this.operationInProgress === false) {
              this.operationInProgress = true
              this.currentOperationName = response.Data.OperationName
            }
          } else {
            clearTimeout(this.timeoutId)
          }
        })
        .catch(e => {
          console.log(e)
        })
      this.timeoutId = setTimeout(this.getOperationInfo, 1000)
    },
    handleConsole() {
      this.consoleDialogVisible = true
      if (this.operationInProgress === false) {
        this.getOperationInfo()
      }
    },
    createConsoleDialogLoading() {
      this.consoleDialogLoading = Loading.service({
        text: '加载中……',
        target: this.$refs.consoleDialog.$el.querySelector('.el-dialog')
      })
    }
  }
}
</script>
