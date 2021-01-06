<template>
  <div class="app-container">
    <div class="filter-container">
      <el-input v-model="listQuery.Name" placeholder="名称" style="width: 200px;" class="filter-item" @keyup.enter.native="handleFilter" />
      <el-select v-model="listQuery.Status" placeholder="状态" clearable style="width: 90px" class="filter-item">
        <el-option v-for="item in statusOptions" :key="item.value" :label="item.name" :value="item.value" />
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
      <el-table-column label="Key 规则">
        <template slot-scope="scope">
          <span>{{ scope.row.Rule }}</span>
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
        <el-form-item label="特征 Key 规则" prop="Rule">
          <el-input v-model="temp.Rule" />
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
import { getList, AddService, UpdateService } from '@/api/service'
import { waves } from '@/directive/waves'

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
      tableKey: 0,
      showDesc: true,
      listQuery: {
        Page: 1,
        Limit: 20,
        Status: statusOptions[0].value, // 默认先查询可用的
        Name: ''
      },
      statusOptions: statusOptions,
      dialogFormVisible: false,
      textMap: {
        'create': '创建维度',
        'edit': '修改维度'
      },
      dialogStatus: 'create',
      temp: {},
      rules: {
        Name: [{ required: true, message: '名称是必填项' }],
        Rule: [{ required: true, message: '特征 key 规则是必填项' }],
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
      getList(this.listQuery).then(response => {
        this.list = response.Data.Items
        this.listLoading = false
      })
    },
    handleFilter() {
      this.listQuery.Page = 1
      this.fetchData()
    },
    resetTemp() {
      this.temp = {
        Id: undefined,
        Status: 1,
        Name: '',
        Desc: '',
        Rule: ''
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

        AddService(this.temp).then(response => {
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

        UpdateService(this.temp).then(response => {
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
