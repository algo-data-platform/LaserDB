<template>
  <div class="app-container">
    <div class="filter-container">
      <el-input v-model="listQuery.Name" placeholder="名称" style="width: 200px;" class="filter-item" @keyup.enter.native="handleFilter" />
      <el-select v-model="listQuery.Status" placeholder="状态" clearable style="width: 90px" class="filter-item">
        <el-option v-for="item in statusOptions" :key="item.value" :label="item.name" :value="item.value" />
      </el-select>
      <el-select v-model="listQuery.Did" class="filter-item" placeholder="请选择">
        <el-option v-for="item in dimOptions" :key="item.Id" :label="item.Name" :value="item.Id" />
      </el-select>
      <el-select v-model="listQuery.Tid" class="filter-item" placeholder="请选择">
        <el-option v-for="item in tableOptions" :key="item.Id" :label="item.Name" :value="item.Id" />
      </el-select>
      <el-button v-waves class="filter-item" type="primary" icon="el-icon-search" @click="handleFilter">
        搜索
      </el-button>
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-edit" @click="handleCreate">
        添加
      </el-button>
      <el-checkbox v-model="showDesc" class="filter-item" style="margin-left:15px;" @change="tableKey=tableKey+1">
        描述
      </el-checkbox>
    </div>
    <el-table
      :key="tableKey"
      v-loading="listLoading"
      :data="list"
      element-loading-text="Loading"
      border
      fit
      highlight-current-row
    >
      <el-table-column align="center" label="ID" width="95">
        <template slot-scope="scope">
          {{ scope.row.Id }}
        </template>
      </el-table-column>
      <el-table-column label="名称">
        <template slot-scope="scope">
          {{ scope.row.Name }}
        </template>
      </el-table-column>
      <el-table-column label="维度名称">
        <template slot-scope="scope">
          <span>{{ scope.row.Dname }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Laser 数据库">
        <template slot-scope="scope">
          <span>{{ scope.row.DatabaseName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Laser 数据表">
        <template slot-scope="scope">
          <span>{{ scope.row.TableName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="描述" v-if="showDesc">
        <template slot-scope="scope">
          <span>{{ scope.row.Desc }}</span>
        </template>
      </el-table-column>
      <el-table-column label="操作" align="center" width="80" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleUpdate(row)">
            修改
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogFormVisible">
      <el-form ref="dataForm" :rules="rules" :model="temp" label-position="left" label-width="120px" style="width: 400px; margin-left:50px;">
        <el-form-item label="名称" prop="Name">
          <el-input v-if="dialogStatus == 'create'" v-model="temp.Name" />
          <span v-if="dialogStatus == 'update'">{{temp.Name}}</span>
        </el-form-item>
        <el-form-item label="维度" prop="Did">
          <el-select v-model="temp.Did" class="filter-item" placeholder="请选择">
            <el-option v-for="item in dimOptions" :key="item.Id" :label="item.Name" :value="item.Id" />
          </el-select>
        </el-form-item>
        <el-form-item label="映射表" prop="Did">
          <el-select v-model="temp.Tid" class="filter-item" placeholder="请选择">
            <el-option v-for="item in tableOptions" :key="item.Id" :label="item.Name" :value="item.Id" />
          </el-select>
        </el-form-item>
        <el-form-item label="状态" prop="Status">
          <el-select v-model="temp.Status" class="filter-item" placeholder="请选择">
            <el-option v-for="item in statusOptions" :key="item.value" :label="item.name" :value="item.value" />
          </el-select>
        </el-form-item>
        <el-form-item label="描述" prop="Desc">
          <el-input v-model="temp.Desc" type="textarea" />
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
import { getList } from '@/api/feature'
import { getList as getDimList } from '@/api/service'
import { getList as getTableList } from '@/api/table'
import { waves } from '@/directive/waves'
import { AddFeature, UpdateFeature } from '@/api/feature'

const statusOptions = [
  {
    name: '可用',
    value: 1
  },
  {
    name: '禁用',
    value: 2
  }
]

export default {
  directives: { waves },
  data() {
    return {
      list: null,
      listLoading: true,
      listQuery: {
        Page: 1,
        Limit: 20,
        Name: '',
        Status: 1,
        Did: undefined,
        Tid: undefined
      },
      showDesc: true,
      tableKey: 1,
      dialogFormVisible: false,
      statusOptions: statusOptions,
      textMap: {
        'create': '创建特征',
        'edit': '修改特征'
      },
      dialogStatus: 'create',
      temp: {},
      rules: {
        Name: [{ required: true, message: '名称是必填项' }],
        Did: [{ required: true, message: '特征维度是必填项' }],
        Tid: [{ required: true, message: '特征映射表是必填项' }]
      },
      dimOptions: [],
      tableOptions: []
    }
  },
  created() {
    this.fetchData()
    this.fetchDimList()
    this.fetchTableList()
    this.resetTemp()
  },
  methods: {
    fetchData() {
      this.listLoading = true
      getList(this.listQuery).then(response => {
        this.list = response.Data.Items
        this.listLoading = false
      })
    },
    fetchDimList() {
      getDimList({ Status: 1 }).then(response => {
        this.dimOptions = response.Data.Items
      })
    },
    fetchTableList() {
      getTableList().then(response => {
        this.tableOptions = response.Data.Items
        for (let i = 0; i < this.tableOptions.length; i++) {
          this.tableOptions[i]['Name'] = this.tableOptions[i].DatabaseName + ':' + this.tableOptions[i].TableName
        }
      })
    },
    handleFilter() {
      this.listQuery.Page = 1
      this.fetchData()
    },
    resetTemp() {
      this.temp = {
        Id: undefined,
        Name: '',
        Desc: '',
        Did: undefined,
        Tid: undefined
      }
    },
    handleCreate() {
      this.resetTemp()
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    createData() {
      this.$refs['dataForm'].validate((valid) => {
        if (!valid) {
          return
        }

        AddFeature(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Created Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchData()
        })
        this.dialogFormVisible = false
      })
    },
    handleUpdate(row) {
      console.info(row)
      this.temp = row
      this.dialogFormVisible = true
      this.dialogStatus = 'update'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    updateData() {
      this.$refs['dataForm'].validate((valid) => {
        if (!valid) {
          return
        }

        UpdateFeature(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchData()
        })
        this.dialogFormVisible = false
      })
    }
  }
}
</script>
