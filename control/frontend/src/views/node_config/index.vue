<template>
  <div class="app-container">
    <div class="filter-container">
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-edit"
        @click="handleCreate">
        添加
      </el-button>
    </div>
    <el-table v-loading="listLoading" :data="list" element-loading-text="Loading" border fit highlight-current-row>
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="节点配置名称">
        <template slot-scope="scope">
          <span>{{ scope.row.Name }}</span>
        </template>
      </el-table-column>
      <el-table-column label="BlockCacheSizeGb">
        <template slot-scope="scope">
          <span>{{ scope.row.BlockCacheSizeGb }}</span>
        </template>
      </el-table-column>
      <el-table-column label="WriteBufferSizeGb">
        <template slot-scope="scope">
          <span>{{ scope.row.WriteBufferSizeGb }}</span>
        </template>
      </el-table-column>
      <el-table-column label="NumShardBits">
        <template slot-scope="scope">
          <span>{{ scope.row.NumShardBits }}</span>
        </template>
      </el-table-column>
      <el-table-column label="HighPriPoolRatio">
        <template slot-scope="scope">
          <span>{{ scope.row.HighPriPoolRatio }}</span>
        </template>
      </el-table-column>
      <el-table-column label="StrictCapacityLimit">
        <template slot-scope="scope">
          <span>{{ capacityLimitText[scope.row.StrictCapacityLimit] }}</span>
        </template>
      </el-table-column>
      <el-table-column label="操作" align="center" width="150" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleUpdate(row)">修改</el-button>
          <el-button type="primary" size="mini" @click="deleteNodeConfig(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>
    <div class="pagination-class">
      <el-pagination :background=true @current-change="handleCurrentChange" :current-page="listQuery.Page"
        :page-size="listQuery.Limit" layout="total, prev, pager, next, jumper" :total="listTotal">
      </el-pagination>
    </div>
    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogFormVisible">
      <el-form :ref="formRefs[0]" :rules="rules" :model="temp" label-position="left" label-width="150px"
        style="width: 400px; margin-left:50px;">
        <el-form-item label="节点配置名称" prop="Name">
          <el-input v-model="temp.Name" v-bind:disabled="dialogStatus=='update'" />
        </el-form-item>
        <el-form-item label="BlockCacheSizeGb" prop="BlockCacheSizeGb">
          <el-input v-model.number="temp.BlockCacheSizeGb" type="number" oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="WriteBufferSizeGb" prop="WriteBufferSizeGb">
          <el-input v-model.number="temp.WriteBufferSizeGb" type="number" oninput="value=value.replace(/[^\d]/g,'')" />
        </el-form-item>
        <el-form-item label="NumShardBits" prop="NumShardBits">
          <el-input v-model.number="temp.NumShardBits" type="number"
            oninput="value=value.replace(/[^0-9|^\\-|^\\.]/g,\'\')" />
        </el-form-item>
        <el-form-item label="HighPriPoolRatio" prop="HighPriPoolRatio">
          <el-input v-model.number="temp.HighPriPoolRatio" type="number"
            oninput="value=value.replace(/[^0-9|^\\-|^\\.]/g,\'\')" />
        </el-form-item>
        <el-form-item label="StrictCapacityLimit" prop="StrictCapacityLimit">
          <el-select v-model.boolean="temp.StrictCapacityLimit" placeholder="请选择">
            <el-option label="true" :value=1></el-option>
            <el-option label="false" :value=0></el-option>
          </el-select>
        </el-form-item>
      </el-form>

      <h3>IO 限速策略</h3>
      <el-divider></el-divider>
      <el-form :ref="formRefs[1]" :model="formRateLimitData" style="margin-left:50px;" label-position="left"
        label-width="150px" size="mini" :rules="rules">
        <el-row :gutter="5" v-for="(item, index) in formRateLimitData.RateLimitStrategy" :key="index">
          <el-col :span="8">
            <el-form-item :label="'时段' + (index + 1)" :prop="'RateLimitStrategy.' + index + '.BeginHour'"
              :rules="rules.RateLimitBeginHour">
              <el-time-select v-model="item.BeginHour" :picker-options="{start: '00:00',step: '01:00',end: '24:00'}"
                placeholder="起始时间">
              </el-time-select>
            </el-form-item>
          </el-col>
          <el-col :span="5">
            <el-form-item label="" label-width="0px" :prop="'RateLimitStrategy.' + index + '.EndHour'"
              :rules="rules.RateLimitEndHour">
              <el-time-select v-model="item.EndHour"
                :picker-options="{start: '00:00',step: '01:00',end: '24:00', minTime: item.BeginHour}"
                placeholder="结束时间">
              </el-time-select>
            </el-form-item>
          </el-col>
          <el-col :span="5">
            <el-form-item label="" label-width="0px" :prop="'RateLimitStrategy.' + index + '.RateBytesPerSec'"
              :rules="rules.RateLimitRateBytesPerSec">
              <el-input v-model.number="item.RateBytesPerSec" type="number" min="0"
                oninput="value=value.replace(/[^\d]/g,'')" placeholder="输入限速值(Byte)"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="3">
            <el-button size="mini" @click.prevent="removeRateLimit(item)">删除</el-button>
          </el-col>
        </el-row>
        <el-form-item>
          <el-button @click="addRateLimit()">新增时段</el-button>
        </el-form-item>
      </el-form>

      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogFormVisible = false">
          取消
        </el-button>
        <el-button type="primary" @click="dialogStatus==='create'? createData() : updateData()">
          提交
        </el-button>
      </div>
    </el-dialog>
  </div>
</template>

<script>
import {
  NodeListConfig,
  NodeAddConfig,
  NodeUpdateConfig,
  DeleteNodeConfig
} from '@/api/node_config'
import { waves } from '@/directive/waves'
import {
  string24HourToNumber,
  number24HourToString,
  clearFormValidate,
  checkForm
} from '@/utils/common'

export default {
  directives: { waves },
  data() {
    return {
      list: null,
      listTotal: undefined,
      currentPage: 1,
      listLoading: true,
      nodeConfigDelete: {
        NodeConfigId: undefined
      },
      listQuery: {
        Page: 1,
        Limit: 20,
        ConfigName: ''
      },
      dialogFormVisible: false,
      textMap: {
        create: '创建节点配置',
        update: '修改节点配置'
      },
      capacityLimitText: {
        0: 'false',
        1: 'true'
      },
      dialogStatus: 'create',
      listNMLoading: true,
      dataConfigNameList: null,
      formRefs: ['formBasicInfo', 'formRateLimit'],
      formRateLimitData: {
        RateLimitStrategy: []
      },
      temp: {},
      rules: {
        Name: [{ required: true, message: '配置名称是必填项' }],
        BlockCacheSizeGb: [
          { required: true, message: 'BlockCacheSizeGb是必填项' },
          { type: 'number', message: '请输入数字格式' }
        ],
        WriteBufferSizeGb: [
          { required: true, message: 'WriteBufferSizeGb是必填项' },
          { type: 'number', message: '请输入数字格式' }
        ],
        NumShardBits: [
          { required: true, message: 'NumShardBits是必填项' },
          { type: 'number', message: '请输入数字格式' }
        ],
        HighPriPoolRatio: [
          { required: true, message: 'HighPriPoolRatio是必填项' },
          { type: 'number', message: '请输入数字格式' }
        ],
        StrictCapacityLimit: [
          { required: true, message: 'StrictCapacityLimit是必填项' },
          { type: 'number', message: '请输入数字格式' }
        ],
        RateLimitBeginHour: [{ required: true, message: '开始时间是必填项' }],
        RateLimitEndHour: [{ required: true, message: '结束时间是必填项' }],
        RateLimitRateBytesPerSec: [
          { required: true, message: '限速值是必填项' },
          { type: 'number', message: '请输入数字格式' }
        ]
      }
    }
  },
  created() {
    this.fetchData()
    this.resetTemp()
  },
  methods: {
    fetchData() {
      this.listLoading = true
      NodeListConfig(this.listQuery).then(response => {
        this.list = response.Data.Items
        this.listTotal = response.Data.Total
        this.listLoading = false
      })
    },
    handleFilter() {
      this.listQuery.Page = 1
      this.fetchData()
    },
    resetTemp() {
      this.temp = this.newNodeConfig()
      this.formRateLimitData.RateLimitStrategy = []
    },
    handleCreate() {
      this.resetTemp()
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.formRefs.forEach(form => {
          clearFormValidate(this, form)
        })
      })
    },
    createData() {
      let formCheckResults = []
      this.formRefs.forEach(form => {
        formCheckResults.push(checkForm(this, form))
      })
      Promise.all(formCheckResults).then(() => {
        if (this.validateRateLimitStrategy()) {
          this.temp.RateLimitStrategy = this.convertFormRateLimitDataToStoredData(
            this.formRateLimitData.RateLimitStrategy
          )
          NodeAddConfig(this.temp).then(response => {
            this.$notify({
              title: 'Success',
              message: 'Created Successfully',
              type: 'success',
              duration: 2000
            })
            this.fetchData()
          })
          this.dialogFormVisible = false
        } else {
          this.$alert('IO 限速策略时段有重叠，请检查！', '提示', {
            confirmButtonText: '确定',
            type: 'error'
          })
        }
      })
    },
    handleUpdate(row) {
      this.temp = row
      this.formRateLimitData.RateLimitStrategy = this.convertStoredRateLimitDataToFormData(
        row.RateLimitStrategy
      )
      this.dialogFormVisible = true
      this.dialogStatus = 'update'
      this.$nextTick(() => {
        this.formRefs.forEach(form => {
          clearFormValidate(this, form)
        })
      })
    },
    deleteNodeConfig(row) {
      this.nodeConfigDelete.NodeConfigId = row.Id
      this.$confirm('确定删除?', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(() => {
        DeleteNodeConfig(this.nodeConfigDelete).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Delete Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchData()
        })
      })
    },
    updateData() {
      let formCheckResults = []
      this.formRefs.forEach(form => {
        formCheckResults.push(checkForm(this, form))
      })
      Promise.all(formCheckResults).then(() => {
        if (this.validateRateLimitStrategy()) {
          this.temp.RateLimitStrategy = this.convertFormRateLimitDataToStoredData(
            this.formRateLimitData.RateLimitStrategy
          )
          NodeUpdateConfig(this.temp).then(response => {
            this.$notify({
              title: 'Success',
              message: 'Update Successfully',
              type: 'success',
              duration: 2000
            })
            this.fetchData()
          })
          this.dialogFormVisible = false
        } else {
          this.$alert('IO 限速策略时段有重叠，请检查！', '提示', {
            confirmButtonText: '确定',
            type: 'error'
          })
        }
      })
    },
    fetchDataTable() {
      this.listNMLoading = true
      NodeListConfig(this.listQuery).then(response => {
        this.dataConfigNameList = response.Data.Items
        this.listNMLoading = false
      })
    },
    handleCurrentChange(val) {
      this.listQuery.Page = val
      this.fetchData()
    },
    validateRateLimitStrategy() {
      const rateLimitStrategy = this.convertFormRateLimitDataToStoredData(
        this.formRateLimitData.RateLimitStrategy
      )
      var SortedMap = require('collections/sorted-map')
      let limits = SortedMap()
      rateLimitStrategy.forEach(item => {
        limits.set(item.BeginHour, item.EndHour)
      })
      let iter = limits.iterator()
      let firstLimit = JSON.parse(JSON.stringify(iter.next()))
      if (firstLimit.done) {
        return true
      }
      let secondLimit = JSON.parse(JSON.stringify(iter.next()))
      while (!secondLimit.done) {
        console.log(firstLimit.value)
        console.log(secondLimit.value)
        if (
          firstLimit.value.key >= firstLimit.value.value ||
          secondLimit.value.key >= secondLimit.value.value ||
          firstLimit.value.value > secondLimit.value.key
        ) {
          return false
        }
        firstLimit = secondLimit
        secondLimit = JSON.parse(JSON.stringify(iter.next()))
      }
      return true
    },
    addRateLimit() {
      console.log(this.formRateLimitData)
      let limits = this.formRateLimitData.RateLimitStrategy
      if (limits === null) {
        return
      }
      limits.push({
        Name: '',
        LimitType: '',
        LimitValue: undefined
      })
    },
    removeRateLimit(item) {
      let limits = this.formRateLimitData.RateLimitStrategy
      if (limits === null) {
        return
      }
      let index = limits.indexOf(item)
      if (index !== -1) {
        limits.splice(index, 1)
      }
    },
    convertFormRateLimitDataToStoredData(formData) {
      let rateLimitStrategy = []
      formData.forEach(item => {
        let limitEntry = {
          BeginHour: string24HourToNumber(item.BeginHour),
          EndHour: string24HourToNumber(item.EndHour),
          RateBytesPerSec: item.RateBytesPerSec
        }
        rateLimitStrategy.push(limitEntry)
      })
      return rateLimitStrategy
    },
    convertStoredRateLimitDataToFormData(sotredData) {
      let formRateLimitData = []
      sotredData.forEach(item => {
        let formLimitEntry = {
          BeginHour: number24HourToString(item.BeginHour),
          EndHour: number24HourToString(item.EndHour),
          RateBytesPerSec: item.RateBytesPerSec
        }
        formRateLimitData.push(formLimitEntry)
      })
      return formRateLimitData
    },
    newNodeConfig() {
      return {
        Id: undefined,
        Name: '',
        BlockCacheSizeGb: undefined,
        WriteBufferSizeGb: undefined,
        NumShardBits: undefined,
        HighPriPoolRatio: undefined,
        StrictCapacityLimit: undefined,
        RateLimitStrategy: []
      }
    }
  }
}
</script>
