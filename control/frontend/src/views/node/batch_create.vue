
<template>
  <div class="app-container">
    <el-row :gutter="20" align="middle">
      <el-col :span="6" style="margin-bottom: 10px">
        <el-form ref="dataForm" :rules="rules" :model="createParams" label-position="left" label-width="120px"
          style="width: 400px; margin-left:10px;">
          <el-form-item label="Laser 数据库" prop="DatabaseName">
            <el-select class="el-select" v-model="createParams.DatabaseName" :loading="listDBLoading" placeholder="请选择"
              @visible-change="fetchDatabase" @change="getDatabaseIdByName(createParams.DatabaseName)">
              <el-option v-for="item in databaseList" :key="item.Id" :label="item.Name" :value="item.Name">
              </el-option>
            </el-select>
          </el-form-item>
          <el-form-item label="Laser 数据表" prop="TableName">
            <el-select class="el-select" v-model="createParams.TableName" :loading="listTableLoading" placeholder="请选择"
              @visible-change="fetchDataTable">
              <el-option v-for="item in tableList" :key="item.Id" :label="item.Name" :value="item.Name">
              </el-option>
            </el-select>
          </el-form-item>
          <el-form-item label="分配方式" prop="Type">
            <el-select class="el-select" v-model="createParams.Type" placeholder="请选择">
              <el-option v-for="item in assignTypes" :key="item.Value" :label="item.Name" :value="item.Value">
              </el-option>
            </el-select>
          </el-form-item>
          <el-form-item label="端口" prop="Port">
            <el-input v-model.number="createParams.Port" type="number" min="0" max="65535"
              oninput="value=value.replace(/[^\d]/g,'')" />
          </el-form-item>
          <el-form-item label="节点配置" prop="ConfigId">
            <el-select class="el-select" v-model="createParams.ConfigId" placeholder="请选择"
              @visible-change="fetchConfigList">
              <el-option v-for="item in configList" :key="item.Id" :label="item.Name" :value="item.Id">
              </el-option>
            </el-select>
          </el-form-item>

          <el-form-item label="启动配置" prop="AnsibleConfigId">
            <ansibleConfigSelection :ansibleConfigId.sync="createParams.AnsibleConfigId">
            </ansibleConfigSelection>
          </el-form-item>
          <el-form-item label="选择机器" prop="SelectedMachines">
            <machineSelection ref="MachineSelection" :selectedMachines.sync="createParams.SelectedMachines">
            </machineSelection>
          </el-form-item>
          <el-button class="filter-item" style="margin-left: 330px;" type="primary" @click="createNodes">
            创建
          </el-button>
        </el-form>
      </el-col>
    </el-row>
  </div>
</template>

<style>
.el-select {
  width: 100%;
}
</style>

<script>
import { ListTable } from '@/api/table'
import { ListDatabase } from '@/api/database'
import { AssignTableShardList } from '@/api/tools'
import { batchStoreNode } from '@/api/node'
import { waves } from '@/directive/waves'
import { newNodeInfo } from '@/utils/common'
import { validatePort, validateIP } from '@/utils/validate'
import machineSelection from './machine_selection'
import ansibleConfigSelection from './ansible_config_selection'

export default {
  name: 'BatchCreate',
  directives: { waves },
  components: {
    machineSelection,
    ansibleConfigSelection
  },
  props: {
    configList: {
      type: Array,
      default() {
        return []
      }
    },
    groupId: {
      type: Number
    },
    nodeList: {
      type: Array,
      default() {
        return []
      }
    }
  },
  data() {
    return {
      assignTypes: [
        { Name: 'ShardSize', Value: 1 },
        { Name: 'ShardKps', Value: 2 },
        { Name: 'ShardNum', Value: 3 }
      ],
      listTableParams: {
        Page: undefined,
        Limit: undefined,
        DatabaseId: undefined
      },
      listDBLoading: true,
      listTableLoading: true,
      databaseList: null,
      tableList: null,
      createParams: {
        DatabaseName: '',
        TableName: '',
        Type: undefined,
        Port: undefined,
        ConfigId: undefined,
        AnsibleConfigId: undefined,
        SelectedMachines: []
      },
      currentMaxNodeId: 0,
      batchCreateParams: {
        Nodes: []
      },
      rules: {
        DatabaseName: [{ required: true, message: '数据库名称是必填项' }],
        TableName: [{ required: true, message: '数据表名称是必填项' }],
        Type: [{ required: true, message: '分配类型是必填项' }],
        Port: [
          { required: true, message: '必填项不能为空', trigger: 'blur' },
          { type: 'number', message: '请输入数字格式', trigger: 'blur' },
          { validator: validatePort, trigger: 'blur' }
        ],
        ConfigId: [{ required: true, message: '必选项不能为空' }],
        SelectedMachines: [{ required: true, message: '必选项不能为空' }],
        AnsibleConfigId: [{ required: true, message: '必选项不能为空' }]
      }
    }
  },
  watch: {
    nodeList(val) {
      let maxNodeId = 0
      val.forEach(nodeInfo => {
        if (nodeInfo.NodeId > maxNodeId) {
          maxNodeId = nodeInfo.NodeId
        }
      })
      this.currentMaxNodeId = maxNodeId
    }
  },
  created() {},
  methods: {
    fetchDatabase() {
      this.listDBLoading = true
      ListDatabase().then(response => {
        this.databaseList = response.Data.Items
        this.listDBLoading = false
      })
    },
    fetchDataTable() {
      this.listTableLoading = true
      ListTable(this.listTableParams).then(response => {
        this.tableList = response.Data.Items
        this.listTableLoading = false
      })
    },
    getDatabaseIdByName(name) {
      for (let index = 0; index < this.databaseList.length; ++index) {
        if (name == this.databaseList[index].Name) {
          this.listTableParams.DatabaseId = this.databaseList[index].Id
          break
        }
      }
    },
    createNodes() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }
        let assginShardParams = {
          DatabaseName: this.createParams.DatabaseName,
          TableName: this.createParams.TableName,
          Type: this.createParams.Type,
          AssignedListNum: this.createParams.SelectedMachines.length
        }
        AssignTableShardList(assginShardParams)
          .then(response => {
            let hosts = new Array()
            this.createParams.SelectedMachines.forEach(option => {
              hosts.push(option)
            })
            hosts.sort(function(a, b) {
              if (a < b) {
                return -1
              }
              if (a > b) {
                return 1
              }
              return 0
            })
            let nodeId = this.currentMaxNodeId + 1
            let index = 0
            response.Data.AssignedShardListInfos.forEach(assginedInfo => {
              let nodeInfo = newNodeInfo()
              nodeInfo.NodeId = nodeId++
              nodeInfo.Active = 1
              nodeInfo.Host = hosts[index++]
              nodeInfo.Port = this.createParams.Port
              nodeInfo.IsEdgeNode = true
              nodeInfo.GroupId = this.groupId
              nodeInfo.ConfigId = this.createParams.ConfigId
              nodeInfo.AnsibleConfigId = this.createParams.AnsibleConfigId
              nodeInfo.FollowerShardList = String(assginedInfo.ShardList)
              this.batchCreateParams.Nodes.push(nodeInfo)
            })
            batchStoreNode(this.batchCreateParams)
              .then(response => {
                this.$notify({
                  title: 'Success',
                  message: 'Created Successfully',
                  type: 'success',
                  duration: 2000
                })
              })
              .finally(e => {
                this.batchCreateParams.Nodes = []
                if (e !== undefined) {
                  throw e
                }
              })
          })
          .finally(e => {
            this.$emit('postCreate')
            if (e !== undefined) {
              throw e
            }
          })
      })
    },
    fetchConfigList(visibleFlag) {
      this.$emit('getConfig', visibleFlag)
    }
  }
}
</script>
