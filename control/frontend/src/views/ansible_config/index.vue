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
    <el-table :key="tableKey" v-loading="listLoading" :data="ansibleConfigList" element-loading-text="Loading" border fit
      highlight-current-row>
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="名称">
        <template slot-scope="scope">
          <span>{{ scope.row.Name }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Roles 配置">
        <template slot-scope="scope">
          <span>{{ scope.row.Roles }}</span>
        </template>
      </el-table-column>
      <el-table-column label="描述">
        <template slot-scope="scope">
          <span>{{ scope.row.Desc }}</span>
        </template>
      </el-table-column>s
      <el-table-column label="操作" align="center" width="225" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleUpdate(row)">修改</el-button>
          <el-button type="primary" size="mini" @click="deleteAnsibleConfig(row)">删除</el-button>
          <el-button type="primary" size="mini" @click="handleClone(row)">克隆</el-button>
        </template>
      </el-table-column>
    </el-table>

    <div class="pagination-class">
      <el-pagination :background=true @current-change="handleCurrentChange" :current-page="queryParams.Page"
        :page-size="queryParams.Limit" layout="total, prev, pager, next, jumper" :total="listTotal">
      </el-pagination>
    </div>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogFormVisible" width="70%">
      <el-form ref="dataForm" :rules="rules" :model="temp" label-position="left" label-width="120px"
        style="width: 700px; margin-left:50px;" size="mini" >
        <el-form-item label="名称" prop="Name">
          <el-input v-model="temp.Name"  />
        </el-form-item>
        <el-form-item label="Roles 配置" prop="Roles">
          <el-input v-model="temp.Roles" />
        </el-form-item>
        <el-form-item label="描述" prop="Desc">
          <el-input v-model="temp.Desc" type="textarea" />
        </el-form-item>
        <div class="title-db-options">
          <h3>Vars 配置</h3>
          <el-divider></el-divider>
        </div>
          <el-row :gutter="5" v-for="(item, index) in temp.Vars" :key="index">
            <el-col :span="9">
              <el-form-item :label="'配置项' + (index + 1)" :prop="'Vars.' + index + '.Name'"
                :rules="rules.Vars" > 
                <el-input  v-model="item.Name" placeholder="请输入配置项名称"></el-input>
              </el-form-item>
            </el-col>
            <el-col :span="12">
              <el-form-item label="" label-width="0px" :prop="'Vars.' + index + '.Value'"
                :rules="rules.Vars">
                <el-input v-model="item.Value" rows="1" type="textarea"  placeholder="请输入配置项值"></el-input>
              </el-form-item>
            </el-col>
            <el-col :span="3">
              <el-button  size="mini"  @click.prevent="removeAnsibleConfigItem(item)" >删除
              </el-button>
            </el-col>
          </el-row>
          <el-form-item>
            <el-button @click="addAnsibleConfigItem()">新增配置项</el-button>
          </el-form-item>
      </el-form>
      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogFormVisible = false">
          取消
        </el-button>
        <el-button type="primary" @click="dialogStatus==='create' || dialogStatus==='clone' ? createAnsibleConfig() : updateAnsibleConfig()">
          提交
        </el-button>
      </div>
    </el-dialog>

  </div>
</template>
 
<script>
import { waves } from '@/directive/waves'
import {
  ListAnsibleConfig,
  AddAnsibleConfig,
  UpdateAnsibleConfig,
  DeleteAnsibleConfig
} from '@/api/ansible_config'

export default {
  directives: { waves },
  data() {
    return {
      listLoading: false,
      tableKey: 0,
      genNextId: this.nextId(),
      ansibleConfigList: undefined,
      listTotal: undefined,
      showDesc: true,
      temp: this.newAnsibleConfigTemp(),
      formData: this.newAnsibleConfigFormData(),
      queryParams: {
        Page: 1,
        Limit: 20
      },
      dialogFormVisible: false,
      textMap: {
        create: '创建Ansible配置',
        update: '修改Ansible配置',
        clone:  '克隆Ansible配置'
      },
      dialogStatus: 'create',
      rules: {
        Name: [{ required: true, message: '名称是必填项' }],
        Roles: [{ required: true, message: 'Roles是必填项' }],
      }
    }
  },
  created() {
    this.fetchAnsibleConfigList()
  },
  methods: {
    fetchAnsibleConfigList() {
      this.listLoading = true
      ListAnsibleConfig(this.queryParams).then(response => {
        this.ansibleConfigList = response.Data.Items
        this.listTotal = response.Data.Total
        this.listLoading = false
      })
    },
    handleCreate() {
      this.temp = this.newAnsibleConfigTemp()
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    handleUpdate(row) {
      this.temp.Id = row.Id
      this.temp.Name = row.Name
      this.temp.Roles = row.Roles

      this.temp.Vars = JSON.parse(row.Vars)
      this.temp.Desc = row.Desc
      this.dialogFormVisible = true
      this.dialogStatus = 'update'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    handleClone(row) {
      this.temp.Name = row.Name
      this.temp.Roles = row.Roles

      this.temp.Vars = JSON.parse(row.Vars)
      this.temp.Desc = row.Desc
      this.dialogFormVisible = true
      this.dialogStatus = 'clone'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    createAnsibleConfig() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }
        this.formData.Name = this.temp.Name
        this.formData.Roles = this.temp.Roles
        this.formData.Desc = this.temp.Desc

        let varsArr = new Array();
        for (let i in this.temp.Vars) {
          let varsObject = new Object();
          varsObject['Name'] = this.temp.Vars[i].Name;
          varsObject['Value'] = this.temp.Vars[i].Value;
          varsArr.push(varsObject)
        }
        this.formData.Vars = JSON.stringify(varsArr) 
        AddAnsibleConfig(this.formData).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Created Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchAnsibleConfigList()
        })
        this.dialogFormVisible = false
      })
    },
    updateAnsibleConfig() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        this.formData.Id = this.temp.Id
        this.formData.Name = this.temp.Name
        this.formData.Roles = this.temp.Roles
        this.formData.Desc = this.temp.Desc
        let varsArr = new Array();
        for (let i in this.temp.Vars) {
          let varsObject = new Object();
          varsObject['Name'] = this.temp.Vars[i].Name;
          varsObject['Value'] = this.temp.Vars[i].Value;
          varsArr.push(varsObject)
        }
        this.formData.Vars = JSON.stringify(varsArr) 
        UpdateAnsibleConfig(this.formData).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchAnsibleConfigList()
        })
        this.dialogFormVisible = false
      })
    },
    handleCurrentChange(val) {
      this.queryParams.Page = val
      this.fetchAnsibleConfigList()
    },
    deleteAnsibleConfig(row) {
      this.$confirm('确定删除?', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(() => {
        DeleteAnsibleConfig(row).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Delete Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchAnsibleConfigList()
        })
      })
    },
    newAnsibleConfigTemp() {
      return {
        Id: 0,
        Name: '',
        Roles: '',
        Desc: '',
        Vars: []
      }
    },
    newAnsibleConfigFormData() {
      return {
        Id: 0,
        Name: '',
        Roles: '',
        Desc: '',
        Vars: ''
      }
    },    
    addAnsibleConfigItem() {
      let items = this.temp.Vars
      if (items == null) {
        return
      }
      items.push({
        Name: '',
        Value: '',
        Key: this.genNextId.next().value
      })
    },
    removeAnsibleConfigItem(item) {
      let items = this.temp.Vars
      if (items == null) {
        return
      }
      let index = items.indexOf(item)
      if (index != -1) {
        items.splice(index, 1)
      }
    },        
    *nextId() {
      let current_id = 0
      while (true) {
        current_id++
        yield current_id
      }
    }    
  }
}
</script>
