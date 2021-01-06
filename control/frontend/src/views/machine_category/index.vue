<template>
  <div class="app-container">
    <div class="filter-container">
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-edit" @click="handleCreate">
        添加
      </el-button>
      <el-checkbox v-model="showDesc" class="filter-item" style="margin-left:15px;" @change="tableKey=tableKey+1">
        描述
      </el-checkbox>
    </div>
    <el-table :key="tableKey" v-loading="listLoading" :data="machineCategoryList" element-loading-text="Loading" border fit
      highlight-current-row>
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="分类名称">
        <template slot-scope="scope">
          <span>{{ scope.row.Name }}</span>
        </template>
      </el-table-column>
      <el-table-column label="描述" v-if="showDesc">
        <template slot-scope="scope">
          <span>{{ scope.row.Desc }}</span>
        </template>
      </el-table-column>
      <el-table-column label="操作" align="center" width="150" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleUpdate(row)">
            修改
          </el-button>
          <el-button type="primary" size="mini" @click="deleteMachineCategory(row)">
            删除
          </el-button>
        </template>
      </el-table-column>
    </el-table>

    <div class="pagination-class">
      <el-pagination :background=true @current-change="handleCurrentChange" :current-page="queryParams.Page"
        :page-size="queryParams.Limit" layout="total, prev, pager, next, jumper" :total="listTotal">
      </el-pagination>
    </div>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogFormVisible">
      <el-form ref="dataForm" :rules="rules" :model="temp" label-position="left" label-width="120px"
        style="width: 400px; margin-left:50px;">
        <el-form-item label="分类名称" prop="Name">
          <el-input v-model="temp.Name" v-bind:disabled="dialogStatus=='update'" />
        </el-form-item>
        <el-form-item label="描述" prop="Desc">
          <el-input v-model="temp.Desc" type="textarea" />
        </el-form-item>
      </el-form>
      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogFormVisible = false">
          取消
        </el-button>
        <el-button type="primary" @click="dialogStatus==='create'? createMachineCategory() : updateMachineCategory()">
          提交
        </el-button>
      </div>
    </el-dialog>

  </div>
</template>
 
<script>
import { waves } from '@/directive/waves'
import {
  ListMachineCategory,
  AddMachineCategory,
  UpdateMachineCategory,
  DeleteMachineCategory
} from '@/api/machine_category'

export default {
  directives: { waves },
  data() {
    return {
      listLoading: false,
      machineCategoryList: undefined,
      listTotal: undefined,
      tableKey: 0,
      showDesc: true,
      temp: this.newMachineCategoryInfo(),
      queryParams: {
        Page: 1,
        Limit: 20
      },
      deleteParams: {
        Id: undefined
      },
      dialogFormVisible: false,
      textMap: {
        create: '创建分类',
        update: '修改分类'
      },
      dialogStatus: 'create',
      rules: {
        Name: [{ required: true, message: '分类名称是必填项' }],
      }
    }
  },
  created() {
    this.fetchMachineCategoryList()
  },
  methods: {
    fetchMachineCategoryList() {
      this.listLoading = true
      ListMachineCategory(this.queryParams).then(response => {
        this.machineCategoryList = response.Data.Items
        this.listTotal = response.Data.Total
        this.listLoading = false
      })
    },
    handleCreate() {
      this.temp = this.newMachineCategoryInfo()
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    handleUpdate(row) {
      this.temp.Id = row.Id
      this.temp.Name = row.Name
      this.temp.Desc = row.Desc
      this.dialogFormVisible = true
      this.dialogStatus = 'update'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    createMachineCategory() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        AddMachineCategory(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Created Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchMachineCategoryList()
        })
        this.dialogFormVisible = false
      })
    },
    updateMachineCategory() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        UpdateMachineCategory(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchMachineCategoryList()
        })
        this.dialogFormVisible = false
      })
    },
    deleteMachineCategory(row) {
      this.$confirm('确定删除?', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(() => {
        this.deleteParams.Id = row.Id
        DeleteMachineCategory(this.deleteParams).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Delete Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchMachineCategoryList()
        })
      })
    },
    handleCurrentChange(val) {
      this.queryParams.Page = val
      this.fetchMachineCategoryList()
    },
    newMachineCategoryInfo() {
      return {
        Id: 0,
        Name: '',
        Desc: ''
      }
    }
  }
}
</script>
