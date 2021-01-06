<template>
  <div>
    <el-row :gutter="20" type="flex" justify="start">
      <el-col>
        <ansibleOperation ref="AnsibleOperation" style="margin-left:10px" @handleBatchCreate="handleBatchCreate"
          @handleEnableBatchOperation="handleEnableBatchOperation" @batchDeleteNodes="batchDeleteNodes"
          @postAnsibleOperation="postAnsibleOperation" :nodesToOperateOn="nodesToOperateOn" :operations="operations"
          :groupName="groupName" :groupId="groupId">
        </ansibleOperation>
      </el-col>
      <br />
      <br />
      <br />
    </el-row>
    <el-row :gutter="20" align="middle">
      <el-col :span="6" style="margin-bottom: 10px" v-for="info in nodeList" :key="info.Id">
        <el-card class="box-card" :style="info | cardStyle">
          <div slot="header" class="clearfix" :style="info | cardHeader">
            <el-checkbox class="filter-item" @change="checked=>onNodeChecked(checked, info)"
              v-if="enableBatchOperation">
            </el-checkbox>
            <span style="font-size: 18px;font-weight: 600">[{{info.NodeId}}] {{info.Host}}:{{info.Port}}</span>
            <el-tag type="success" effect="dark" size="mini" v-if="info.Master">主</el-tag>
          </div>
          <!-- <el-row v-for="value in info.Attrs" style="margin: 10px 0px">
            <el-col :span="8">
              <span>{{value.Name}}</span>
            </el-col>
            <el-col :span="16">
              <span>{{value.Value}}</span>
            </el-col>
          </el-row> -->
          <el-row style:="margin: 10px 0px">
            <el-col :span="8">
              <span> 读 KPS: </span>
            </el-col>
            <el-col :span="16">
              <span> {{info.ReadKps}} </span>
            </el-col>
          </el-row>
          <br />
          <el-row style:="margin: 10px 0px">
            <el-col :span="8">
              <span> 写 KPS: </span>
            </el-col>
            <el-col :span="16">
              <span> {{info.WriteKps}} </span>
            </el-col>
          </el-row>
          <div>
            <el-divider style="margin: 10px"><i class="el-icon-more"></i></el-divider>
          </div>
          <el-row justify="center">
            <el-col :span="6">
              <el-tooltip class="item" effect="dark" content="启用节点" placement="top">
                <el-button type="success" icon="el-icon-video-play" circle
                  @click="handleNodeCommand(operations.enable.Name,info)"></el-button>
              </el-tooltip>
            </el-col>
            <el-col :span="6">
              <el-tooltip class="item" effect="dark" content="暂停节点" placement="top">
                <el-button type="warning" icon="el-icon-video-pause" circle
                  @click="handleNodeCommand(operations.disable.Name,info)"></el-button>
              </el-tooltip>
            </el-col>
            <el-col :span="6">
              <el-tooltip class="item" effect="dark" content="详细信息" placement="top">
                <el-button type="success" icon="el-icon-s-operation" circle @click="handleShowDetail(info)"></el-button>
              </el-tooltip>
            </el-col>
            <el-col :span="6">
              <el-dropdown :hide-on-click="true" @command="command=>handleNodeCommand(command,info)">
                <el-button type="warning" icon="el-icon-more" circle></el-button>
                <el-dropdown-menu slot="dropdown">
                  <el-dropdown-item :command="operations.start.Name">启动</el-dropdown-item>
                  <el-dropdown-item :command="operations.stop.Name">停止</el-dropdown-item>
                  <el-dropdown-item :command="operations.restart.Name">重启</el-dropdown-item>
                  <el-dropdown-item :command="operations.upgrade.Name">更新</el-dropdown-item>
                  <el-dropdown-item :command="operations.delete.Name">删除</el-dropdown-item>
                </el-dropdown-menu>
              </el-dropdown>
            </el-col>
          </el-row>
        </el-card>
      </el-col>
      <el-col :span="6" style="margin-bottom: 10px">
        <el-card class="box-card" style="text-align: center;height: 270px">
          <el-button type="text" class="button" icon="el-icon-plus" style="font-size: 8rem;height: 220px"
            @click="handleCreate">
          </el-button>
        </el-card>
      </el-col>
    </el-row>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogFormVisible" width="90%">
      <div>
        <h3>节点信息</h3>
        <el-divider></el-divider>
        <el-form ref="dataForm" :rules="rules" :model="temp" label-position="left" label-width="120px"
          style="width: 700px; margin-left:50px;">
          <el-form-item label="所属组" prop="GroupName">
            <el-select class="el-select" v-model="temp.GroupName" :loading="listGroupLoading" placeholder="请选择"
              @visible-change="fetchGroupList" @change="getGroupIdByName(temp.GroupName)">
              <el-option v-for="item in groupList" :key="item.Id" :label="item.Name" :value="item.Name">
              </el-option>
            </el-select>
          </el-form-item>
          <el-form-item label="节点Id" prop="NodeId">
            <el-input v-model.number="temp.NodeId" type="number" min="1" oninput="value=value.replace(/[^\d]/g,'')" />
          </el-form-item>
          <el-form-item label="地址" prop="Host">
            <machineSelection ref="MachineSelection" :multipleSelection="false" :selectedMachines.sync="temp.Host">
            </machineSelection>
          </el-form-item>
          <el-form-item label="端口" prop="Port">
            <el-input v-model.number="temp.Port" type="number" min="0" max="65535"
              oninput="value=value.replace(/[^\d]/g,'')" />
          </el-form-item>
          <el-form-item label="配置" prop="ConfigName">
            <el-select class="el-select" v-model="temp.ConfigName" :loading="listConfigLoading" placeholder="请选择"
              @visible-change="fetchConfigList" @change="getConfigIdByName(temp.ConfigName)">
              <el-option v-for="item in configList" :key="item.Id" :label="item.Name" :value="item.Name">
              </el-option>
            </el-select>
          </el-form-item>
          <el-form-item label="启动配置" prop="AnsibleConfigId">
            <ansibleConfigSelection :ansibleConfigId.sync="temp.AnsibleConfigId">
            </ansibleConfigSelection>
          </el-form-item>
          <el-form-item label="主节点" prop="Master">
            <el-switch v-model="temp.Master" :active-text="yesNoText[1]" :inactive-text="yesNoText[0]"
              :active-value="true" :inactive-value="false" active-color="#13ce66" inactive-color="#ff4949"
              @change="roleChanged">
            </el-switch>
          </el-form-item>
          <el-form-item label="边缘节点" prop="IsEdgeNode">
            <el-switch v-model="temp.IsEdgeNode" :active-text="yesNoText[1]" :inactive-text="yesNoText[0]"
              :active-value="true" :inactive-value="false" active-color="#13ce66" inactive-color="#ff4949"
              v-bind:disabled="temp.Master == true">
            </el-switch>
          </el-form-item>
          <el-form-item label="激活" prop="Active">
            <el-switch v-model="temp.Active" :active-text="yesNoText[1]" :inactive-text="yesNoText[0]" :active-value=1
              :inactive-value=0 active-color="#13ce66" inactive-color="#ff4949">
            </el-switch>
          </el-form-item>
          <el-form-item label="主分片列表" prop="LeaderShardList">
            <el-input v-model="tempLeaderShards" type="textarea" v-bind:disabled="temp.Master == false"
              @change="removeDuplicateLeaderShards" />
          </el-form-item>

          <div v-if="temp.IsEdgeNode == false">
            <el-form-item label="从分片列表" prop="FollowerShardList">
               <el-input v-model="tempFollowerShards" type="textarea" v-bind:disabled="temp.Master == true"
                @change="removeDuplicateFollowerShards" />
            </el-form-item>
          </div>

          <div v-else>
          <el-form-item label="从分片列表" prop="FollowerShardList">

            <div style="width: 640px;" label-position="left" >
              <el-form v-if="tempEdgeNodeFollowerShards.ShardsList.length > 0" ref="followerShardListForm" :model="tempEdgeNodeFollowerShards">
                <el-row :gutter="5" v-for="(tableShards, index) in tempEdgeNodeFollowerShards.ShardsList" :key="index">
                  <el-col :span="20">
                    <el-form-item :label="tableShards.Table" label-width="250px" style="margin-top:20px;" :prop="'ShardsList.' + index + '.Shards'" :rules="followerShardListRules.Shards"> 
                      <el-input  v-model="tableShards.Shards" type="textarea" clearable @change="removeDuplicateFollowerShards" ></el-input>
                    </el-form-item>
                  </el-col>
                  <el-col :span="3">
                    <el-button  size="mini" style="margin-top:30px" @click.prevent="handleDeleteTable(tableShards)" >删除
                    </el-button>
                  </el-col>
                </el-row>
              </el-form>

              <el-form v-if="addNewTableFlag" ref="selectTableForm" :model="selectTable" :rules="SelectTableRules" style="margin-top:20px" >
                <el-row :gutter="5">
                  <el-col :span="9">
                    <el-form-item prop="nodeSelectedTable">
                      <el-cascader v-model="selectTable.nodeSelectedTable" :options="tableOption"
                        @visible-change="fetchTableList" @change="judgeTableExists">
                      </el-cascader>
                    </el-form-item>
                  </el-col>
                  <el-col :span="11">
                    <el-form-item style="margin-left:8px" prop="nodeSelectedTableShards" >
                       <el-input v-model="selectTable.nodeSelectedTableShards" type="textarea" clearable
                        @change="removeSelectedTableFollowerDuplicateShards" />
                    </el-form-item>
                  </el-col>
                  <el-col :span="3">
                    <el-button size="mini" style="margin-top:10px" @click.prevent="handelDeleteSelectedTable" >删除
                    </el-button>
                  </el-col>
                </el-row>
              </el-form>
            </div>

            <el-button size="mini" style="margin-left: 0px; margin-top: 30px;" @click="handleAddNewTable(addNewTableFlag)">
                添加数据表
            </el-button>

          </el-form-item>
          </div>
          <el-form-item label="描述" prop="desc">
            <el-input v-model="temp.Desc" type="textarea" />
          </el-form-item>
        </el-form>
      </div>

      <div slot="default" class="dialog-footer">
        <el-button @click="dialogFormVisible = false">
          取消
        </el-button>
        <el-button type="primary" @click="dialogStatus==='create'? createNode() : updateNode()">
          提交
        </el-button>
      </div>

      <div v-if="dialogShardsVisible == true">

        <h3>分片信息</h3>
        <el-divider></el-divider>
        <el-row>
          <el-col :key="shard.id" v-for="shard in currentNodeShards" :xs="2" :sm="4" :md="3" :lg="2" :xl="1">
            <shardView :shardInfo="shard"></shardView>
          </el-col>
        </el-row>
      </div>
    </el-dialog>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogBatchCreateVisible" width="40%">
      <batchCreate @getConfig="fetchConfigList" @postCreate="postBatchCreate" :configList="configList"
        :groupId="groupId" :nodeList="nodeList">
      </batchCreate>
    </el-dialog>

  </div>
</template>

<style>
.box-card {
  height: 270px;
}
.el-select {
  width: 100%;
}
</style>

<script>
import { waves } from '@/directive/waves'
import shardView from './shard'
import { ListGroup } from '@/api/group'
import { storeNode, updateNode, deleteNode, batchDeleteNodes } from '@/api/node'
import { NodeListConfig } from '@/api/node_config'
import { validatePort, validateIP } from '@/utils/validate'
import { newNodeInfo } from '@/utils/common'
import batchCreate from './batch_create'
import ansibleOperation from './ansible_operation'
import machineSelection from './machine_selection'
import ansibleConfigSelection from './ansible_config_selection'
import { ListTable } from '@/api/table'

export default {
  directives: { waves },
  components: {
    shardView,
    batchCreate,
    ansibleOperation,
    machineSelection,
    ansibleConfigSelection
  },
  props: {
    nodeList: {
      type: Array,
      default() {
        return []
      }
    },
    shards: {
      type: Object,
      default() {
        return {}
      }
    },
    relations: {
      type: Object,
      default() {
        return {}
      }
    },
    shardList: {
      type: Object,
      default() {
        return {}
      }
    },
    groupName: {
      type: String
    },
    groupId: {
      type: Number
    }
  },
  filters: {
    cardStyle: function(node) {
      if (node.Active !== 1) {
        return 'background:#adabab'
      }

      if (node.Status !== 1) {
        return 'background:#e6ab5d'
      }
      return 'background:#ffffff'
    },
    cardHeader: function(node) {
      if (node.IsAvailable === false) {
        return 'color:#df912f'
      }
      return 'color:#000000'
    }
  },
  data() {
    return {
      dialogShardsVisible: false,
      currentNodeShards: [],
      dialogFormVisible: false,
      dialogBatchCreateVisible: false,
      textMap: {
        create: '新增节点',
        update: '',
        batchCreate: '批量添加边缘节点'
      },
      dialogStatus: 'create',
      statusText: {
        0: '禁用',
        1: '启用'
      },
      yesNoText: {
        0: '否',
        1: '是'
      },
      temp: newNodeInfo(),
      tempLeaderShards: "",
      tempFollowerShards: "",
      tempEdgeNodeFollowerShards: {
        ShardsList:[]
      },
      TablelistParams: {
        Page: 1,
        Limit: 0,
        DatabaseName: "",
        TableName: ""
      },
      tableList:[],
      tableOption:[],
      selectTable: {
        nodeSelectedTable: [],
        nodeSelectedTableShards: "",
      },
      addNewTableFlag: false,
      listGroupLoading: false,
      groupList: [],
      groupListParams: {
        ShowDetails: true
      },
      listConfigLoading: false,
      configList: [],
      nodeDeleteParams: {
        Id: 0
      },
      enableBatchOperation: false,
      selectedNodes: new Object(),
      nodesToOperateOn: new Map(),
      batchDeleteNodeParams: {
        Ids: []
      },
      operations: {
        start: { Name: 'start', Tags: 'start' },
        stop: { Name: 'stop', Tags: 'stop' },
        enable: { Name: 'enable', Tags: 'enable' },
        disable: { Name: 'disable', Tags: 'disable' },
        restart: { Name: 'restart', Tags: 'restart' },
        upgrade: { Name: 'upgrade', Tags: 'upgrade' },
        delete: { Name: 'delete', Tags: 'delete' },
        add: { Name: 'add', Tags: 'add' },
        batchDelete: { Name: 'batchDelete', Tags: 'batchDelete' },
        batchAdd: { Name: 'batchAdd', Tags: 'batchAdd' },
        changeRole: { Name: 'changeRole', Tags: 'changeRole' }
      },
      selectedMachines: [],
      rules: {
        GroupName: [
          { required: true, message: '必选项不能为空', trigger: 'blur' }
        ],
        NodeId: [
          { required: true, message: '必填项不能为空', trigger: 'blur' }
        ],
        Host: [
          { required: true, message: '必填项不能为空', trigger: 'blur' },
          { validator: validateIP, trigger: 'blur' }
        ],
        Port: [
          { required: true, message: '必填项不能为空', trigger: 'blur' },
          { type: 'number', message: '请输入数字格式', trigger: 'blur' },
          { validator: validatePort, trigger: 'blur' }
        ],
        Weight: [{ required: true, message: '必填项不能为空' }],
        ConfigName: [{ required: true, message: '必选项不能为空' }],
        AnsibleConfigId: [{ required: true, message: '必选项不能为空' }]
      },
      followerShardListRules: {
        Shards: [
          { required: true, message: '必选项不能为空', trigger: "blur"}
        ]
      },
      SelectTableRules: {
        nodeSelectedTable: [
          { type: 'array', required: true, message: '请选择数据表', trigger: 'blur'}
        ],
        nodeSelectedTableShards: [
          { required: true, message: '必选项不能为空', trigger: "blur"}
        ]
      }
    }
  },
  methods: {
    shardHandler(nodeId) {
      this.currentNodeShards = []
      if (typeof this.shardList[this.groupName][nodeId] === 'undefined') {
        return false
      }
      const shardIds = this.shardList[this.groupName][nodeId]
      shardIds.forEach(id => {
        if (typeof this.shards[id] === 'undefined') {
          return false
        }
        const shardInfo = this.shards[id]
        const relation = this.relations[shardInfo.shardId]
        const relations = []
        if (relation.leader !== id) {
          relations.push(this.shards[relation.leader])
        }
        relation.follower.forEach(followerId => {
          if (followerId === id) {
            return false
          }
          relations.push(this.shards[followerId])
        })
        shardInfo['relations'] = relations
        this.currentNodeShards.push(shardInfo)
        this.currentNodeShards.sort(function(shard1, shard2) {
          return shard1.shardId - shard2.shardId
        })
      })
    },
    handleShowDetail(info) {
      this.temp = info
      this.dialogStatus = 'update'
      this.updateTempShardsStruct()
      this.shardHandler(this.temp.NodeId)
      this.dialogShardsVisible = true
      this.dialogFormVisible = true
    },
    handleCreate() {
      this.temp = newNodeInfo()
      this.clearTempShards()
      this.dialogShardsVisible = false
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    handleAddNewTable(addNewTableFlag) {
      if (addNewTableFlag) {
        this.addNewTableToNode()
      } else {
        this.addNewTableFlag = true
      }
    },
    handleBatchCreate() {
      this.dialogStatus = 'batchCreate'
      this.dialogBatchCreateVisible = true
    },
    postBatchCreate() {
      this.dialogBatchCreateVisible = false
    },
    stringfyNonEdgeNodeShards(shardsList) {
      let tempShards = {
        ShardsList: []
      }
      tempShards.ShardsList.push({
        Table: "default",
        Shards: shardsList
      })
      return JSON.stringify(tempShards)
    },
    formatTempShardList() {
      if (!this.temp.IsEdgeNode) {
        this.temp.LeaderShardList = this.stringfyNonEdgeNodeShards(this.tempLeaderShards)
        this.temp.FollowerShardList = this.stringfyNonEdgeNodeShards(this.tempFollowerShards)
      } else {
        this.temp.FollowerShardList = JSON.stringify(this.tempEdgeNodeFollowerShards)
      }
    },
    edgeNodeAddNewTableAndValidateShards() {
      if (this.tempEdgeNodeFollowerShards.ShardsList.length > 0) {
        let validShardList = false
        this.$refs['followerShardListForm'].validate(valid => {
          if (!valid) {
            validShardList = false
          } else {
            validShardList = true
          }
        })
        if (!validShardList) {
          return false
        }
      }      

      if (this.addNewTableFlag) {
        let success = this.addNewTableToNode()
        if (!success) {
          return false
        }
        this.addNewTableFlag = false
      }
      return true
    },
    createNode() {
      if (this.temp.IsEdgeNode) {
        let success = this.edgeNodeAddNewTableAndValidateShards()
        if (!success) {
          return
        }
      }

      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }
        this.formatTempShardList()
        storeNode(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Created Successfully',
            type: 'success',
            duration: 2000
          })
        })
        this.dialogFormVisible = false
      })
    },
    updateNode() {
      if (this.temp.IsEdgeNode) {
        let success = this.edgeNodeAddNewTableAndValidateShards()
        if (!success) {
          return
        }
      }

      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }
        this.formatTempShardList()
        updateNode(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
        })
        this.dialogFormVisible = false
      })
    },
    deleteNode(info) {
      this.$confirm('确认要删除该节点？', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      })
        .then(() => {
          this.$refs.AnsibleOperation.checkNodeCanStop()
            .then(() => {
              this.nodeDeleteParams.Id = info.Id
              deleteNode(this.nodeDeleteParams).then(response => {
                this.$notify({
                  title: 'Success',
                  message: 'Delete Successfully',
                  type: 'success',
                  duration: 2000
                })
                this.removeCheckedNode(this.nodeDeleteParams.Id)
              })
            })
            .catch(e => {
              if (e.message !== this.$refs.AnsibleOperation.userAbortMessage) {
                throw e
              }
            })
        })
        .catch(() => {})
    },
    batchDeleteNodes() {
      this.$confirm('确认要删除选中的节点？', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      })
        .then(() => {
          this.$refs.AnsibleOperation.checkNodeCanStop()
            .then(() => {
              this.batchDeleteNodeParams.Ids = []
              Object.keys(this.selectedNodes).forEach(id => {
                this.batchDeleteNodeParams.Ids.push(parseInt(id))
              })
              batchDeleteNodes(this.batchDeleteNodeParams).then(response => {
                this.$notify({
                  title: 'Success',
                  message: 'Delete Successfully',
                  type: 'success',
                  duration: 2000
                })
                this.batchDeleteNodeParams.Ids.forEach(id => {
                  this.removeCheckedNode(id)
                })
              })
            })
            .catch(e => {
              if (e.message !== this.$refs.AnsibleOperation.userAbortMessage) {
                throw e
              }
            })
        })
        .catch(() => {})
    },
    fetchGroupList(visibleFlag) {
      if (visibleFlag) {
        this.listGroupLoading = true
        ListGroup(this.groupListParams)
          .then(response => {
            this.groupList = response.Data.Items
          })
          .finally(() => {
            this.listGroupLoading = false
          })
      }
    },
    getGroupIdByName(groupName) {
      for (let index = 0; index < this.groupList.length; ++index) {
        if (groupName === this.groupList[index].Name) {
          this.temp.GroupId = this.groupList[index].Id
          if (
            this.groupList[index].NodeConfigId !== 0 &&
            this.temp.ConfigId === 0
          ) {
            this.temp.ConfigId = this.groupList[index].NodeConfigId
            this.temp.ConfigName = this.groupList[index].NodeConfigName
          }
          break
        }
      }
    },
    fetchConfigList(visibleFlag) {
      if (visibleFlag) {
        this.listConfigLoading = true
        NodeListConfig()
          .then(response => {
            this.configList = response.Data.Items
          })
          .finally(() => {
            this.listConfigLoading = false
          })
      }
    },
    getConfigIdByName(configName) {
      for (let index = 0; index < this.configList.length; ++index) {
        if (configName === this.configList[index].Name) {
          this.temp.ConfigId = this.configList[index].Id
          break
        }
      }
    },
    roleChanged(isMaster) {
      if (isMaster) {
        this.temp.IsEdgeNode = false
      }
    },
    clearTempShards() {
      this.tempLeaderShards = ""
      this.tempFollowerShards = ""
      this.tempEdgeNodeFollowerShards.ShardsList = []
      this.selectTable.nodeSelectedTable = []
      this.selectTable.nodeSelectedTableShards = ""
      this.addNewTableFlag = false
    },
    parseShardsForNonEdgeNodeShards(rawShardList) {
      let tempShards = {
        ShardsList: []
      }
      try {
        tempShards = JSON.parse(rawShardList)
        return tempShards.ShardsList[0].Shards
      } catch(e) {
        return rawShardList
      }
    },
    updateTempShardsStruct() {
      this.clearTempShards()
      if (!this.temp.IsEdgeNode) {
        this.tempLeaderShards = this.parseShardsForNonEdgeNodeShards(this.temp.LeaderShardList)
        this.tempFollowerShards = this.parseShardsForNonEdgeNodeShards(this.temp.FollowerShardList)
      } else {
        if (this.temp.FollowerShardList === '') {
          return
        }        
        try {
          this.tempEdgeNodeFollowerShards = JSON.parse(this.temp.FollowerShardList)
        } catch(e) {
          this.$set(this.tempEdgeNodeFollowerShards.ShardsList, '0', {
            "Table" : "default",
            "Shards": this.temp.FollowerShardList
          })
        }
      }
    },
    removeDuplicatedShards(shards) {
      let shardArray = []
      shards.split(',').forEach(shardId => {
        shardArray.push(shardId.trim())
      })
      return String(Array.from(new Set(shardArray)))
    },
    removeDuplicateLeaderShards() {
      this.tempLeaderShards = this.removeDuplicatedShards(this.tempLeaderShards)
    },
    removeDuplicateFollowerShards() {
      if (!this.temp.IsEdgeNode) {
        this.tempFollowerShards = this.removeDuplicatedShards(this.tempFollowerShards)
      } else {
        for (let i = 0; i < this.tempEdgeNodeFollowerShards.ShardsList.length; i++) {
          this.tempEdgeNodeFollowerShards.ShardsList[i]["Shards"] = 
          this.removeDuplicatedShards(this.tempEdgeNodeFollowerShards.ShardsList[i]["Shards"])
        }
      }
    },
    removeSelectedTableFollowerDuplicateShards() {
      this.selectTable.nodeSelectedTableShards = this.removeDuplicatedShards(this.selectTable.nodeSelectedTableShards)
    },
    handleEnableBatchOperation(val) {
      this.enableBatchOperation = val
      this.selectedNodes = {}
    },
    onNodeChecked(checked, node) {
      let key = node.Id
      if (checked) {
        this.$set(this.selectedNodes, key, node)
      } else {
        this.$delete(this.selectedNodes, key)
      }
    },
    removeCheckedNode(id) {
      this.$delete(this.selectedNodes, id)
    },
    handleNodeCommand(command, node) {
      this.nodesToOperateOn.clear()
      this.nodesToOperateOn.set(node.Id, node)
      switch (command) {
        case this.operations.enable.Name:
          this.$refs.AnsibleOperation.enable()
          break
        case this.operations.disable.Name:
          this.$refs.AnsibleOperation.disable()
          break
        case this.operations.start.Name:
          this.$refs.AnsibleOperation.start()
          break
        case this.operations.stop.Name:
          this.$refs.AnsibleOperation.stop()
          break
        case this.operations.restart.Name:
          this.$refs.AnsibleOperation.restart()
          break
        case this.operations.upgrade.Name:
          this.$refs.AnsibleOperation.upgrade()
          break
        case this.operations.delete.Name:
          this.deleteNode(node)
          break
      }
    },
    postAnsibleOperation() {
      this.nodesToOperateOn = new Map(Object.entries(this.selectedNodes))
    },
    fetchTableList() {
      ListTable(this.TablelistParams).then(response => {
        this.tableList = response.Data.Items
        this.tableOption = []

        let databases = []
        this.tableList.forEach(table => {
          databases.push(table.DatabaseName)
        })
        databases = Array.from(new Set(databases))

        databases.forEach(database => {
          let databaseOption = {
            value: database,
            label: database,
            children: []
          }
          this.tableOption.push(databaseOption)
        })

        this.tableList.forEach(table => {
          for (let i = 0; i < this.tableOption.length; i++) {
            if (table.DatabaseName === this.tableOption[i].value) {
              let tableItemOption = {
                value: table.Name,
                label: table.Name,
              }
              this.tableOption[i].children.push(tableItemOption)
            }
          }
        })
      })
    },
    judgeTableExists() {
      let tableName = this.selectTable.nodeSelectedTable[0] + ":" + this.selectTable.nodeSelectedTable[1]
      return this.tempEdgeNodeFollowerShards.ShardsList.some(tableShards => {
        if (tableShards.Table === tableName) {
          return true
        }
      })
    },
    addNewTableToNode() {
      // 判断表名，分片信息是否有效
      let valid = false
      this.$refs['selectTableForm'].validate(validTable => {
          if (!validTable) {
            valid = false
          } else {
            valid = true
          }
      })
      if (!valid) {
        return false
      }
      // 判断数据表是否与已有的重复
      if (this.judgeTableExists() === true) {
        this.$message({
              message: "该表的分片信息已存在，请直接修改或重新添加!!",
              type: 'error'
            })
        return false
      }

      // 将新表的分片信息添加到分片列表中
      this.tempEdgeNodeFollowerShards.ShardsList.push({
        "Table" : this.selectTable.nodeSelectedTable[0] + ":" + this.selectTable.nodeSelectedTable[1],
        "Shards": this.selectTable.nodeSelectedTableShards
      })
      this.removeDuplicateFollowerShards()

      // 清空数据表和分片信息
      this.selectTable.nodeSelectedTable = []
      this.selectTable.nodeSelectedTableShards = ""
      return true
    },
    handleDeleteTable(tableShards) {
      for (let i = 0; i < this.tempEdgeNodeFollowerShards.ShardsList.length; i++) {
        if (tableShards.Table === this.tempEdgeNodeFollowerShards.ShardsList[i].Table) {
          this.tempEdgeNodeFollowerShards.ShardsList.splice(i, 1)
          this.removeDuplicateFollowerShards()
          return
        }
      }
    },
    handelDeleteSelectedTable() {
      this.addNewTableFlag = false
      this.selectTable.nodeSelectedTable = []
      this.selectTable.nodeSelectedTableShards = ""
    }
  },
  watch: {
    shardList(val) {
      if (this.dialogShardsVisible && this.dialogFormVisible) {
        this.shardHandler(this.temp.NodeId)
      }
    },
    shards(val) {
      if (this.dialogShardsVisible && this.dialogFormVisible) {
        this.shardHandler(this.temp.NodeId)
      }
    },
    relations(val) {
      if (this.dialogShardsVisible && this.dialogFormVisible) {
        this.shardHandler(this.temp.NodeId)
      }
    },
    nodeList(val) {
      val.forEach(item => {
        if (item.Id in this.selectedNodes) {
          this.selectedNodes[item.Id] = item
        }
      })
    },
    selectedNodes: {
      handler(val) {
        this.nodesToOperateOn = new Map(Object.entries(val))
      },
      deep: true
    }
  }
}
</script>
