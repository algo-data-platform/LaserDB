<template>
  <div class="app-container">
    <div class="filter-container">
      <el-form ref="searchForm" :model="queryParams" label-position="left" style="margin-left:20px;" >
        <el-row :gutter="15">
          <el-col :span="9">
            <el-form-item label="机器分类" label-width="70px" prop="SearchMachineCategory">
              <el-input v-model="queryParams.CategoryName" clearable/>
            </el-form-item>
          </el-col>
          <el-col :span="9">
            <el-form-item label="机器IP" label-width="60px" prop="SearchMachineIp">
              <el-input v-model="queryParams.MachineIp" clearable/>
            </el-form-item>
          </el-col>
          <el-col :span="3">
            <el-button class="filter-item" style="width: 90px; margin-left: 10px; " type="primary" icon="el-icon-search" @click="searchMachine()">
              搜索
            </el-button>
          </el-col>
        </el-row>
      </el-form>
    
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-edit" @click="handleCreate">
        添加
      </el-button>
      <el-checkbox v-model="showDesc" class="filter-item" style="margin-left:15px;" @change="tableKey=tableKey+1">
        描述
      </el-checkbox>
    </div>
    <el-table :key="tableKey" v-loading="listLoading" :data="machineList" element-loading-text="Loading" border fit
      highlight-current-row>
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="分类ID" v-if="false">
        <template slot-scope="scope">
          <span>{{ scope.row.CategoryId }}</span>
        </template>
      </el-table-column>
      <el-table-column label="分类名称">
        <template slot-scope="scope">
          <span>{{ scope.row.MachineCategoryName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="IP">
        <template slot-scope="scope">
          <span>{{ scope.row.Ip }}</span>
        </template>
      </el-table-column>
      <el-table-column label="CPU核数">
        <template slot-scope="scope">
          <span>{{ scope.row.CpuCoreNumber }}</span>
        </template>
      </el-table-column>
      <el-table-column label="内存容量[GB]">
        <template slot-scope="scope">
          <span>{{ scope.row.MemorySizeGb }}</span>
        </template>
      </el-table-column>
      <el-table-column label="描述" v-if="showDesc">
        <template slot-scope="scope">
          <span>{{ scope.row.Desc }}</span>
        </template>
      </el-table-column>
      <el-table-column label="操作" align="center" width="150" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleUpdate(row)">修改</el-button>
          <el-button type="primary" size="mini" @click="deleteMachine(row)">删除</el-button>
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

        <el-form-item label="分类名称" prop="MachineCategoryName">
          <el-select class="el-select" v-model="temp.MachineCategoryName" :loading="listLoading" placeholder="请选择"
            @visible-change="fetchMachineCategory" @change="getMachineCategoryIdByName(temp.MachineCategoryName)" 
            v-bind:disabled="dialogStatus=='update'">
            <el-option v-for="item in machineCategoryList" :key="item.Id" :label="item.Name" :value="item.Name">
            </el-option>
          </el-select>
        </el-form-item>

        <el-form-item v-if="dialogStatus=='update'" label="Ip" prop="Ip">
          <el-input v-model="temp.Ip" v-bind:disabled="dialogStatus=='update'" />
        </el-form-item>
        <el-form-item v-else label="Ip" prop="Ip">
          <el-input type="textarea" :rows="10" v-model="temp.Ip" />
        </el-form-item>

        <el-form-item label="CPU核数" prop="CpuCoreNumber">
          <el-input v-model="temp.CpuCoreNumber"  />
        </el-form-item>
        <el-form-item label="内存容量[GB]" prop="MemorySizeGb">
          <el-input v-model="temp.MemorySizeGb" />
        </el-form-item>
        <el-form-item label="描述" prop="Desc">
          <el-input v-model="temp.Desc" type="textarea" />
        </el-form-item>
      </el-form>
      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogFormVisible = false">
          取消
        </el-button>
        <el-button type="primary" @click="dialogStatus==='create'? createMachine() : updateMachine()">
          提交
        </el-button>
      </div>
    </el-dialog>

  </div>
</template>
 
<script>
import { waves } from '@/directive/waves'
import {
  ListMachine,
  AddMachine,
  UpdateMachine,
  DeleteMachine
} from '@/api/machine'
import { ListMachineCategory } from '@/api/machine_category'

export default {
  directives: { waves },
  data() {
    return {
      listLoading: false,
      machineList: undefined,
      machineCategoryList: undefined,
      listTotal: undefined,
      tableKey: 0,
      showDesc: true,
      temp: this.newMachineInfo(),
      queryParams: {
        CategoryName: '',
        MachineIp: '',
        Page: 1,
        Limit: 20
      },
      dialogFormVisible: false,
      textMap: {
        create: '创建机器资源',
        update: '修改机器资源'
      },
      dialogStatus: 'create',
      rules: {
        MachineCategoryName: [{ required: true, message: '分类名称是必填项' }],
        Ip: [{ required: true, message: 'Ip是必填项' }],
        CpuCoreNumber: [{ required: true, message: 'CPU核数是必填项' }],
        MemorySizeGb: [{ required: true, message: '内存容量[GB]是必填项' }]
      }
    }
  },
  created() {
    this.fetchMachineList()
  },
  methods: {
    fetchMachineList() {
      this.listLoading = true
      ListMachine(this.queryParams).then(response => {
        this.machineList = response.Data.Items
        this.listTotal = response.Data.Total
        this.listLoading = false
      })
    },
    handleCreate() {
      this.temp = this.newMachineInfo()
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    handleUpdate(row) {
      this.temp.Id = row.Id
      this.temp.CategoryId = row.CategoryId
      this.temp.MachineCategoryName = row.MachineCategoryName
      this.temp.Ip = row.Ip
      this.temp.CpuCoreNumber = row.CpuCoreNumber
      this.temp.MemorySizeGb = row.MemorySizeGb
      this.temp.Desc = row.Desc
      this.dialogFormVisible = true
      this.dialogStatus = 'update'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    createMachine() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }
        AddMachine(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Created Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchMachineList()
        })
        this.dialogFormVisible = false
      })
    },
    fetchMachineCategory(visibleFlag) {
      console.log(visibleFlag)
      if (visibleFlag) {
        this.listLoading = true
        ListMachineCategory().then(response => {
          this.machineCategoryList = response.Data.Items
          this.listLoading = false
        })
      }
    },
    getMachineCategoryIdByName(name) {
      for (let index = 0; index < this.machineCategoryList.length; ++index) {
        if (name === this.machineCategoryList[index].Name) {
          this.temp.CategoryId = this.machineCategoryList[index].Id
          break
        }
      }
    },
    updateMachine() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        UpdateMachine(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchMachineList()
        })
        this.dialogFormVisible = false
      })
    },
    handleCurrentChange(val) {
      this.queryParams.Page = val
      this.fetchMachineList()
    },
    deleteMachine(row) {
      this.$confirm('确定删除?', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(() => {
        DeleteMachine(row).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Delete Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchMachineList()
        })
      })
    },
    searchMachine() {
      this.queryParams.Page = 1
      this.fetchMachineList()
    },
    newMachineInfo() {
      return {
        Id: 0,
        MachineCategoryName: '',
        CategoryId: 0,
        Ip: '',
        CpuCoreNumber: '',
        MemorySizeGb: '',
        Desc: ''
      }
    }
  }
}
</script>
