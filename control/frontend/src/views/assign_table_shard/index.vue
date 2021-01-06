<template>
  <div class="app-container">
    <el-row :gutter="20" align="middle">
      <el-col :span="6" style="margin-bottom: 10px">
        <el-form ref="dataForm" :rules="rules" :model="assignParams" label-position="left" label-width="120px"
          style="width: 400px; margin-left:10px;">
          <el-form-item label="Laser 数据库" prop="DatabaseName">
            <el-select class="el-select" v-model="assignParams.DatabaseName" :loading="listDBLoading" placeholder="请选择"
              @visible-change="fetchDatabase" @change="getDatabaseIdByName(assignParams.DatabaseName)">
              <el-option v-for="item in databaseList" :key="item.Id" :label="item.Name" :value="item.Name">
              </el-option>
            </el-select>
          </el-form-item>
          <el-form-item label="Laser 数据表" prop="TableName">
            <el-select class="el-select" v-model="assignParams.TableName" :loading="listTableLoading" placeholder="请选择"
              @visible-change="fetchDataTable">
              <el-option v-for="item in tableList" :key="item.Id" :label="item.Name" :value="item.Name">
              </el-option>
            </el-select>
          </el-form-item>
          <el-form-item label="分配方式" prop="Type">
            <el-select class="el-select" v-model="assignParams.Type" placeholder="请选择">
              <el-option v-for="item in assignTypes" :key="item.Value" :label="item.Name" :value="item.Value">
              </el-option>
            </el-select>
          </el-form-item>
          <el-form-item label="分配列表个数" prop="AssignedListNum">
            <el-input v-model.number="assignParams.AssignedListNum" type="number" min="0"
              oninput="value=value.replace(/[^\d]/g,'')"></el-input>
          </el-form-item>
          <el-button class="filter-item" style="margin-left: 330px;" type="primary" @click="assignTableShard">
            分配
          </el-button>
        </el-form>
      </el-col>
    </el-row>

    <p>分配后的分片</p>
    <el-divider style="margin: 10px"></el-divider>
    <el-row :gutter="20" align="middle">
      <el-col :span="6" style="margin-bottom: 10px">
        <el-button class="filter-item" style="margin-left: 10px;" type="primary" @click="copyAllLists">
          复制分片列表
        </el-button>
      </el-col>
    </el-row>
    <el-row :gutter="20" align="middle">
      <el-table :key="tableKey" v-loading="listLoading" :data="assignResult.AssignedShardListInfos"
        element-loading-text="Loading" show-summary :summary-method="getSummaries" border fit highlight-current-row
        style="margin-left: 10px;">
        <el-table-column align="center" label="ID" width="120" type="index"></el-table-column>
        <el-table-column align="center" label="分片列表">
          <template slot-scope="scope">
            <span>{{ JSON.stringify(scope.row.ShardList) }}</span>
          </template>
        </el-table-column>
        <el-table-column align="center" label="占用空间">
          <template slot-scope="scope">
            <span>{{ bytesToSize(scope.row.SizeSum) }}</span>
          </template>
        </el-table-column>
        <el-table-column align="center" label="KPS">
          <template slot-scope="scope">
            <span>{{ scope.row.KpsSum }}</span>
          </template>
        </el-table-column>
      </el-table>
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
import { waves } from '@/directive/waves'
import { Message } from 'element-ui'

export default {
  directives: { waves },
  data() {
    return {
      assignResult: {
        TotalShardListInfo: [],
        AssignedShardListInfos: []
      },
      listLoading: false,
      assignTypes: [
        { Name: 'ShardSize', Value: 1 },
        { Name: 'ShardKps', Value: 2 },
        { Name: 'ShardNum', Value: 3 }
      ],
      assignParams: {
        DatabaseName: '',
        TableName: '',
        Type: undefined,
        AssignedListNum: undefined
      },
      listTableParams: {
        Page: undefined,
        Limit: undefined,
        DatabaseId: undefined
      },
      listDBLoading: true,
      listTableLoading: true,
      databaseList: null,
      tableList: null,
      tableKey: 0,
      rules: {
        DatabaseName: [{ required: true, message: '数据库名称是必填项' }],
        TableName: [{ required: true, message: '数据表名称是必填项' }],
        Type: [{ required: true, message: '分配类型是必填项' }],
        AssignedListNum: [{ required: true, message: '分配列表数量是必填项' }]
      }
    }
  },
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
    assignTableShard() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        this.listLoading = true
        AssignTableShardList(this.assignParams)
          .then(response => {
            this.assignResult = response.Data
            console.log(this.assignResult)
          })
          .finally(() => {
            this.listLoading = false
          })
      })
    },
    getSummaries(param) {
      const sum = []
      sum.push('总计')
      sum.push(JSON.stringify(this.assignResult.TotalShardListInfo.ShardList))
      sum.push(this.bytesToSize(this.assignResult.TotalShardListInfo.SizeSum))
      sum.push(this.assignResult.TotalShardListInfo.KpsSum)
      return sum
    },
    copyAllLists() {
      let lists = new Array()
      this.assignResult.AssignedShardListInfos.forEach(resultEntry => {
        lists.push(resultEntry.ShardList)
      })
      this.$copyText(JSON.stringify(lists)).then(
        function(e) {
          Message({
            showClose: true,
            message: '复制成功',
            type: 'success',
            center: true,
            duration: 1000
          })
        },
        function(e) {
          Message({
            showClose: true,
            message: '复制失败',
            type: 'warning',
            center: true,
            duration: 1000
          })
          console.log(e)
        }
      )
    },
    bytesToSize(bytes) {
      if (bytes === 0) return '0 B'
      let k = 1024
      let sizes = ['B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB']
      let i = Math.floor(Math.log(bytes) / Math.log(k))
      return (bytes / Math.pow(k, i)).toFixed(1) + ' ' + sizes[i]
    }
  }
}
</script>
