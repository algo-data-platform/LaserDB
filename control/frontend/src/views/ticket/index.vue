<template>
  <div class="app-container">
    <div class="filter-container">
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-edit"
        @click="handleCreate">
        添加
      </el-button>
    </div>
      <el-table :key="ticketKey" v-loading="listLoading" :data="ticketList" element-loading-text="Loading" border fit
      highlight-current-row >
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="创建者">
        <template slot-scope="scope">
          <span>{{ scope.row.Creator }}</span>
        </template>
      </el-table-column>
      <el-table-column label="SDK 类型">
        <template slot-scope="scope">
          <span>{{ scope.row.SDKType.toString() }}</span>
        </template>
      </el-table-column>
      <el-table-column label="业务描述">
        <template slot-scope="scope">
          <span>{{ scope.row.BusinessDescription }}</span>
        </template>
      </el-table-column>
      <el-table-column label="数据库名">
        <template slot-scope="scope">
          <span>{{ scope.row.DatabaseName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="数据表名">
        <template slot-scope="scope">
          <span>{{ scope.row.TableName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="数据过期时间(MS)">
        <template slot-scope="scope">
          <span>{{ scope.row.DataExpirationTime }}</span>
        </template>
      </el-table-column>
      <el-table-column label="数据总量(GB)">
        <template slot-scope="scope">
          <span>{{ scope.row.DataSize }}</span>
        </template>
      </el-table-column>
      <el-table-column label="value 大小(KB)">
        <template slot-scope="scope">
          <span>{{ scope.row.ValueSize }}</span>
        </template>
      </el-table-column>
      <el-table-column label="调用接口">
        <template slot-scope="scope">
          <span>{{ scope.row.Command.toString() }}</span>
        </template>
      </el-table-column>
      <el-table-column label="分区数">
        <template slot-scope="scope">
          <span>{{ scope.row.PartitionNum }}</span>
        </template>
      </el-table-column>
      <el-table-column label="当前状态" >
        <template slot-scope="scope" >
          <span>{{ statusText[scope.row.Status] }}</span>
        </template>
      </el-table-column>
      <el-table-column label="操作" align="center" width="150" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleUpdate(row)" >
            {{ textTicketStatusMap[row.TicketStatus] }}
          </el-button>
          <el-button type="primary" size="mini" @click="deleteTicket(row)">删除</el-button>
        </template>
      </el-table-column>
      <el-table-column label="操作" align="center" width="80" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleProcess(row)" v-bind:disabled="row.Status==1 || row.Status==5">
            审核
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    <div class="pagination-class">
      <el-pagination :background=true @current-change="handleCurrentChange" :current-page="queryParams.Page"
        :page-size="queryParams.Limit" layout="total, prev, pager, next, jumper" :total="listTotal">
      </el-pagination>
    </div>

    <el-dialog :title="textDialogStatusMap[dialogStatus]" :visible.sync="dialogFormVisible">
      <el-form ref="dataForm" :rules="rules" :model="temp" label-position="left" label-width="120px"
        style="width: 400px; margin-left:50px;" v-bind:disabled="temp.TicketStatus=='watch'" >
        <el-form-item label="SDK 类型" prop="SDKType" >
          <li style="list-style:none;" v-for="(item, index) in SDKTypes" >
            <input type="checkbox" v-bind:id="index" v-bind:value=item.type v-model="temp.SDKType" />
            <label for="index">{{ item.type }}</label>
          </li>
        </el-form-item>
        <el-form-item label="业务线" prop="BusinessLine">
          <el-input v-model="temp.BusinessLine" />
        </el-form-item>
        <el-form-item label="业务描述" prop="BusinessDescription">
          <el-input v-model="temp.BusinessDescription" />
        </el-form-item>
        <el-form-item label="业务 KR" prop="BusinessKR">
          <el-input v-model="temp.BusinessKR" />
        </el-form-item>
        <el-form-item label="读接口人(邮箱前缀)" prop="ReadContactPersion">
          <el-input v-model="temp.ReadContactPersion" />
        </el-form-item>
        <el-form-item label="写接口人(邮箱前缀)" prop="WriteContactPersion">
          <el-input v-model="temp.WriteContactPersion" />
        </el-form-item>
        <el-form-item label="批量导入接口人(邮箱前缀)" prop="ImportDataContactPersion">
          <el-input v-model="temp.ImportDataContactPersion" />
        </el-form-item>
        <el-form-item label="Laser 数据库" prop="DatabaseName">
          <el-select class="el-select" v-model="temp.DatabaseName" filterable
            :loading="listDBLoading" placeholder="请选择"
            @visible-change="fetchDatabase" @change="getDatabaseIdByName(temp.DatabaseName)" @blur="inputDatabaseName" >
            <el-option v-for="item in databaseList" :key="item.Id" :label="item.Name" :value="item.Name"></el-option>
          </el-select>
        </el-form-item>
        <el-form-item label="Laser 数据表" prop="TableName" >
          <el-select class="el-select" v-model="temp.TableName" filterable
            :loading="listDTLoading" placeholder="请选择"
            @visible-change="fetchDataTable" @blur="inputTableName" >
            <el-option v-for="item in dataTableList" :key="item.Id" :label="item.Name" :value="item.Name"></el-option>
          </el-select>
        </el-form-item>
        <el-form-item label="读 QPS" prop="ReadQPS">
          <el-input v-model.number="temp.ReadQPS" type="number" min="1"
            oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="写 QPS" prop="WriteQPS">
          <el-input v-model.number="temp.WriteQPS" type="number" min="1"
            oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="请求延时(P999)" prop="RequestDelayLimit">
          <el-input v-model.number="temp.RequestDelayLimit" type="number" min="1"
          oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="数据过期时间(MS)" prop="DataExpirationTime">
          <el-input v-model.number="temp.DataExpirationTime" type="number" min="1"
          oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="数据总量(GB)" prop="DataSize">
          <el-input v-model.number="temp.DataSize" type="number" min="1"
          oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="value 大小预估(KB)" prop="ValueSize">
          <el-input v-model.number="temp.ValueSize" type="number" min="1"
          oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="宕机影响范围" prop="CrashInfluence">
          <el-input v-model="temp.CrashInfluence" />
        </el-form-item>
        <el-form-item label="调用接口" prop="Command" >
          <li style="list-style:none;" v-for="(item, index) in commandList" >
            <input type="checkbox" v-bind:id="index" v-bind:value="item.Name" v-model="temp.Command" v-on:click="updateCommandChoosed(item)"  />
            <label for="index">{{ item.Name }}</label>
          </li>
        </el-form-item>
        <el-form-item label="单次 mset/mget 对应的 key 个数" prop="KeyNum" v-show="multiCommandChoosed==true" >
          <el-input v-model.number="temp.KeyNum" type="number" min="1"
          oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="邮件抄送人员(邮箱前缀，以','分隔)" prop="DockingPersonnel" >
          <el-input v-model="temp.DockingPersonnel" />
        </el-form-item>
        <el-form-item label="分区数(申请人无需填写)" prop="PartitionNum">
          <el-input v-model.number="temp.PartitionNum" type="number" min="1" v-bind:disabled=true
          oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="当前状态" prop="Status" >
           <input type="radio" id="keep" value=1 v-model.number="temp.Status">
           <label for="keep">保存</label>
           <input type="radio" id="submit" value=2 v-model.number="temp.Status">
           <label for="submit">提交</label>
        </el-form-item>
      </el-form>
      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogFormVisible = false">
          取消
        </el-button>
        <el-button type="primary" @click="dialogStatus==='create'? createTicket() : updateTicket()">
          提交
        </el-button>
      </div>
    </el-dialog>
    <el-dialog :title="textDialogStatusMap[dialogStatus]" :visible="dialogProcessFormVisible">
      <el-form ref="processDataForm" :rules="rules" :model="tempProcess" label-position="left" label-width="120px"
        style="width: 400px; margin-left:50px;">
        <el-form-item label="处理状态" prop="TicketAcceptStatus">
          <el-switch v-model="tempProcess.TicketAcceptStatus" :active-text="textProcessStatusMap['accept']" :inactive-text="textProcessStatusMap['reject']"
          :active-value=1 :inactive-value=0 active-color="#13ce66" inactive-color="#13ce66">
          </el-switch>
        </el-form-item>
        <el-form-item label="数据库名" prop="DatabaseName">
          <el-input v-model="tempProcess.DatabaseName" v-bind:disabled="dialogStatus=='process'"/>
        </el-form-item>
        <el-form-item label="数据表名" prop="TableName">
          <el-input v-model="tempProcess.TableName" v-bind:disabled="dialogStatus=='process'" />
        </el-form-item>
       
        <el-form-item label="数据中心" prop="DcName" >
          <el-select class="el-select" v-model="tempProcess.DcName" :loading="listDcLoading" placeholder="请选择"
            @visible-change="fetchDcData" @change="getDcIdByName(tempProcess.DcName)"   v-bind:disabled="tempProcess.TicketAcceptStatus==0">
            <el-option v-for="item in dcList" :key="item.Id" :label="item.Name" :value="item.Name">
            </el-option>
          </el-select>
        </el-form-item>

        <el-form-item label="数据表配置" prop="TableConfigName" >
          <el-select class="el-select" v-model="tempProcess.TableConfigName" :loading="listConfigLoading" placeholder="请选择"
            @visible-change="fetchConfigData" @change="getConfigIdByName(tempProcess.TableConfigName)"   v-bind:disabled="tempProcess.TicketAcceptStatus==0">
            <el-option v-for="item in configList" :key="item.Id" :label="item.Name" :value="item.Name">
            </el-option>
          </el-select>
        </el-form-item>
        <el-form-item label="分区数" prop="PartitionNum" >
          <el-input v-model.number="tempProcess.PartitionNum" type="number" min="1" v-bind:disabled="tempProcess.TicketAcceptStatus==0"
            oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="数据过期时间" prop="DataExpirationTime">
          <el-input v-model.number="tempProcess.DataExpirationTime" type="number"
          oninput="value=value.replace(/[^\d]/g,'')" v-bind:disabled="tempProcess.TicketAcceptStatus==0"/>
        </el-form-item>
        <el-form-item label="驳回原因" prop="RejectReason" v-show="tempProcess.TicketAcceptStatus==0" >
          <el-input v-model="tempProcess.RejectReason" />
        </el-form-item>
      </el-form>
      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogProcessFormVisible = false">
          取消
        </el-button>
        <el-button type="primary" @click="ticketProcess()">
          提交
        </el-button>
      </div>
    </el-dialog>
  </div>
</template>


<style>
.el-select {
  width: 100%;
}
</style>

<script>
import {
  ListTicket,
  AddTicket,
  UpdateTicket,
  ProcessTicket,
  DeleteTicket
} from '@/api/ticket'

import { ListDc } from "@/api/dc";
import { ListDatabase } from '@/api/database'
import { ListTable, ListCommands } from '@/api/table'
import { ListTableConfig } from '@/api/table_config'
import { waves } from '@/directive/waves'
import { validPartitionNumber } from '@/utils/validate'
import { mapGetters } from 'vuex'

export default {
  directives: { waves },
  data() {
    return {
      checkedNames: [],
      dialogFormVisible: false,
      dialogProcessFormVisible: false,
      keyNumVisible: false,
      listLoading: true,
      ticketList: undefined,
      ticketDelete: {
        TicketId: undefined
      },
      commandList: undefined,
      listTotal: undefined,
      listDBLoading: true,
      listDTLoading: true,
      listConfigLoading: true,
      listDcLoading: true,
      dcList: undefined,
      ticketKey: 0,
      textDialogStatusMap: {
        create: '创建工单',
        update: '修改工单',
        process: '工单审核',
      },
      textTicketStatusMap: {
        update: '修改',
        watch: '查看',
      },
      textProcessStatusMap: {
        reject: '驳回',
        accept: '接受',
      },
      dialogStatus: 'create',
      multiCommandChoosed: false,
      flagMget: false,
      flagMset: false,
      flagMgetDetail: false,
      flagMsetDetail: false,
      databaseList: null,
      dataTableList: null,
      configList: null,
      temp: this.newTicketInfo(),
      tempProcess: this.newProcessTicketInfo(),
      fetchDTName :{
        Page : undefined,
        Limit : undefined,
        DatabaseId : undefined,
      },
      queryParams: {
        Loader: "",
        Page: 1,
        Limit: 20
      },
      rules: {
        SDKType: [{ required: true, message: 'sdk 是必填项' }],
        DatabaseName: [{ required: true, message: '数据库名称是必填项' }],
        TableName: [{ required: true, message: '数据表名称是必填项' }],
        ValueSize: [{ required: true, message: 'value 大小预估是必填项' }],
        Command: [{ required: true, message: '调用接口是必填项' }],
        Status: [{ required: true, message: '当前状态是必填项' }],
        TableConfigName: [{ required: true, message: '数据表配置是必填项' }],
        DcName: [{ required: true, message: '数据中心是必填项' }],
        PartitionNum: [
          { required: true, message: '分区数是必填项' },
          { type: 'number', message: '请输入数字格式' },
          { validator: validPartitionNumber, trigger: 'blur' }
        ]
      },
      SDKTypes:[
        { type: 'go' },
        { type: 'c++' },
        { type: 'java' }
      ],
      statusText: {
        1: '保存',
        2: '待审核',
        3: '被驳回',
        4: '处理失败',
        5: '处理完成',
      },
    }
  },
  created() {
    this.resetTemp()
    this.resetTempProcess()
    this.fetchTicketList()
    this.fetchCommands()
  },
  methods: {
    resetTemp() {
      this.temp = this.newTicketInfo()
    },
    resetTempProcess() {
      this.tempProcess = this.newProcessTicketInfo()
    },
    fetchTicketList() {
      this.listLoading = true
      ListTicket(this.queryParams).then(response => {
        this.ticketList = response.Data.Items
        this.listTotal = response.Data.Total
        this.listLoading = false
        this.updateParams()
      })
    },
    fetchCommands() {
      ListCommands().then(response => {
        this.commandList = response.Data.Items
      })
    },
    handleCreate() {
      this.temp = this.newTicketInfo()
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    handleUpdate(row) {
      this.temp.Id = row.Id
      this.temp.Creator = row.Creator
      this.temp.SDKType = row.SDKType
      this.temp.BusinessLine = row.BusinessLine
      this.temp.BusinessDescription = row.BusinessDescription
      this.temp.BusinessKR = row.BusinessKR
      this.temp.ReadContactPersion = row.ReadContactPersion
      this.temp.WriteContactPersion = row.WriteContactPersion
      this.temp.ImportDataContactPersion = row.ImportDataContactPersion
      this.temp.DatabaseName = row.DatabaseName
      this.temp.TableName = row.TableName
      this.temp.ReadQPS = row.ReadQPS
      this.temp.WriteQPS = row.WriteQPS
      this.temp.RequestDelayLimit = row.RequestDelayLimit
      this.temp.DataExpirationTime = row.DataExpirationTime
      this.temp.DataSize = row.DataSize
      this.temp.ValueSize = row.ValueSize
      this.temp.CrashInfluence = row.CrashInfluence
      this.temp.Command = row.Command
      this.temp.KeyNum = row.KeyNum
      this.temp.DockingPersonnel = row.DockingPersonnel
      this.temp.PartitionNum = row.PartitionNum
      this.temp.Status = row.Status
      this.temp.TicketStatus = row.TicketStatus
      this.dialogFormVisible = true
      this.dialogStatus = 'update'
      this.updateParams()
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    deleteTicket(row) {
      this.ticketDelete.TicketId = row.Id
      this.$confirm('确定删除?', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(() => {
        DeleteTicket(this.ticketDelete).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Delete Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchTicketList()
        })
      })
    },
    handleProcess(row) {
      this.tempProcess = this.newProcessTicketInfo()
      this.dialogProcessFormVisible = true
      this.dialogStatus = 'process'
      this.tempProcess.Handler = this.$store.state.user.email
      this.tempProcess.DatabaseName = row.DatabaseName
      this.tempProcess.TableName = row.TableName
      this.tempProcess.DataExpirationTime = row.DataExpirationTime
      this.$nextTick(() => {
        this.$refs['processDataForm'].clearValidate()
      })
    },
    createTicket() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }
        this.temp.Creator = this.$store.state.user.email
        AddTicket(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Created Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchTicketList()
        })
        this.dialogFormVisible = false
        this.updateParams()
      })
    },
    updateTicket() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }
        UpdateTicket(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchTicketList()
        })
        this.dialogFormVisible = false
        this.updateParams()
      })
    },
    ticketProcess() {
      this.$refs['processDataForm'].validate(valid => {
        if (!valid) {
          return
        }
        this.shardListLoading = true
        ProcessTicket(this.tempProcess).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Process Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchTicketList()
        })
        this.dialogProcessFormVisible = false
      })
    },
    handleCurrentChange(val) {
      this.queryParams.Page = val
      this.fetchTicketList()
    },
    fetchDatabase() {
      this.listDBLoading = true
      ListDatabase().then(response => {
        this.databaseList = response.Data.Items
        this.temp.TableName = ""
        this.listDBLoading = false
      })
    },
    fetchDataTable() {
      this.listDTLoading = true;
      ListTable(this.fetchDTName).then(response => {
        this.dataTableList = response.Data.Items;
        this.listDTLoading = false;
      });
    },
    getDatabaseIdByName(name) {
      for (let index = 0; index < this.databaseList.length; ++index) {
        if (name == this.databaseList[index].Name) {
          this.fetchDTName.DatabaseId = this.databaseList[index].Id
          break
        }
      }
    },
    fetchConfigData(visibleFlag) {
      if (visibleFlag) {
        this.listConfigLoading = true
        ListTableConfig()
          .then(response => {
            this.configList = response.Data.Items
          })
          .finally(() => {
            this.listConfigLoading = false
          })
      }
    },
    fetchDcData(visibleFlag) {
      if (visibleFlag) {
        this.listDcLoading = true;
        ListDc()
          .then((response) => {
            this.dcList = response.Data.Items;
          })
          .finally(() => {
            this.listDcLoading = false;
          });
      }
    },
    getDcIdByName(name) {
      for (let index = 0; index < this.dcList.length; ++index) {
        if (name == this.dcList[index].Name) {
          this.tempProcess.DcId = this.dcList[index].Id;
          break;
        }
      }
    },
    getConfigIdByName(name) {
      for (let index = 0; index < this.configList.length; ++index) {
        if (name == this.configList[index].Name) {
          this.tempProcess.TableConfigId = this.configList[index].Id
          break
        }
      }
    },
    // 支持动态显示 key 个数, 以及动态显示 '修改'/'查看'
    updateCommandChoosed(item) {
      this.multiCommandChoosed = false
      if('mget' == item.Name) {
        this.flagMget = !this.flagMget
      }
      if('mset' == item.Name) {
        this.flagMset = !this.flagMset
      }
      if('mgetDetail' == item.Name) {
        this.flagMgetDetail = !this.flagMgetDetail
      }
      if('msetDetail' == item.Name) {
        this.flagMsetDetail = !this.flagMsetDetail
      }
      if (this.flagMget || this.flagMset || this.flagMgetDetail || this.flagMsetDetail){
	      this.multiCommandChoosed = !this.multiCommandChoosed
	    }
    },
    updateParams(e) {
      for(var i = 0; i < this.ticketList.length; i++){
        var canUpdate = (this.$store.state.user.email==this.ticketList[i].Creator && this.ticketList[i].Status==1) ||
                         this.$store.state.user.isAdmin
        this.ticketList[i].TicketStatus = (canUpdate == true) ? 'update' : 'watch'
      }
      for(var i = 0; i < this.temp.Command.length; i++){
        if ('mget' == this.temp.Command[i]) {
          this.flagMget = true
        }
        if ('mset' == this.temp.Command[i]) {
          this.flagMset = true
        }
        if ('mgetDetail' == this.temp.Command[i]) {
          this.mgetDetail = true
        }
        if ('mgetDetail' == this.temp.Command[i]) {
          this.msetDetail = true
        }
      }
      if (this.flagMget || this.flagMset || this.flagMgetDetail || this.flagMsetDetail){
	      this.multiCommandChoosed = !this.multiCommandChoosed
	    }
    },
    // 支持对话框既能下拉，又可以输入的功能
    inputDatabaseName(e) {
      this.temp.DatabaseName = e.target.value
  	},
    inputTableName(e) {
      this.temp.TableName = e.target.value
  	},
    newTicketInfo() {
      return {
        Id: 0,
        Creator: undefined,
        SDKType : [],
        BusinessLine : undefined,
        BusinessDescription : undefined,
        BusinessKR : undefined,
        ReadContactPersion : undefined,
        WriteContactPersion : undefined,
        ImportDataContactPersion : undefined,
        DatabaseName : undefined,
        TableName : undefined,
        ReadQPS : undefined,
        WriteQPS : undefined,
        RequestDelayLimit : undefined,
        DataExpirationTime : 0,
        DataSize : undefined,
        ValueSize : undefined,
        CrashInfluence : undefined,
        Command : [],
        KeyNum : undefined,
        DockingPersonnel : undefined,
        PartitionNum : 1,
        Status : undefined,
        TicketStatus: 'update',
      }
    },
    newProcessTicketInfo() {
      return {
        Handler : undefined,
        TicketAcceptStatus:1,
        DatabaseName : undefined,
        TableName : undefined,
        DcName: undefined,
        DcId: 0,
        DataExpirationTime : 0,
        TableConfigId : 1,
        TableConfigName : 'default',
        PartitionNum : 100,
        RejectReason : undefined,
      }
    },
  }
}

</script>
