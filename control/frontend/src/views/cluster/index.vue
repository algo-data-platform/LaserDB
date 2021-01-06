<template>
  <div class="app-container">
    <div class="filter-container">
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-edit" @click="handleCreate"
        v-bind:disabled="listTotal != 0">
        添加
      </el-button>
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-upload"
        :loading="consulSynchronizing" @click="synchronizeClusterInfoToConsul">
        同步
      </el-button>
      <el-checkbox v-model="showDesc" class="filter-item" style="margin-left:15px;" @change="tableKey=tableKey+1">
        描述
      </el-checkbox>
    </div>
    <el-table :key="tableKey" v-loading="listLoading" :data="clusterList" element-loading-text="Loading" border fit
      highlight-current-row>
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="集群名">
        <template slot-scope="scope">
          <span>{{ scope.row.Name }}</span>
        </template>
      </el-table-column>
      <el-table-column label="别名">
        <template slot-scope="scope">
          <span>{{ scope.row.Alias }}</span>
        </template>
      </el-table-column>
      <el-table-column label="分片总数">
        <template slot-scope="scope">
          <span>{{ scope.row.ShardTotal }}</span>
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

    <div class="pagination-class">
      <el-pagination :background=true @current-change="handleCurrentChange" :current-page="queryParams.Page"
        :page-size="queryParams.Limit" layout="total, prev, pager, next, jumper" :total="listTotal">
      </el-pagination>
    </div>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogFormVisible">
      <el-form ref="dataForm" :rules="rules" :model="temp" label-position="left" label-width="120px"
        style="width: 400px; margin-left:50px;">
        <el-form-item label="集群名" prop="Name">
          <el-input v-model="temp.Name" v-bind:disabled="dialogStatus=='update'" />
        </el-form-item>
        <el-form-item label="别名" prop="Alias">
          <el-input v-model="temp.Alias" />
        </el-form-item>
        <el-form-item label="分片总数" prop="ShardTotal">
          <el-input v-model.number="temp.ShardTotal" type="number" min="1" oninput="value=value.replace(/[^\d]/g,'')"
            v-bind:disabled="dialogStatus=='update'" />
        </el-form-item>
        <el-form-item label="描述" prop="Desc">
          <el-input v-model="temp.Desc" type="textarea" />
        </el-form-item>
      </el-form>
      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogFormVisible = false">
          取消
        </el-button>
        <el-button type="primary" @click="dialogStatus==='create'? createCluster() : updateCluster()">
          提交
        </el-button>
      </div>
    </el-dialog>

  </div>
</template>
 
<script>
import { waves } from '@/directive/waves'
import {
  ListCluster,
  AddCluster,
  UpdateCluster,
  SynchronizeClusterInfoToConsul
} from '@/api/cluster'

export default {
  directives: { waves },
  data() {
    return {
      listLoading: false,
      clusterList: undefined,
      listTotal: undefined,
      tableKey: 0,
      showDesc: true,
      temp: this.newClusterInfo(),
      queryParams: {
        Page: 1,
        Limit: 20
      },
      dialogFormVisible: false,
      textMap: {
        create: '创建集群',
        update: '修改集群'
      },
      dialogStatus: 'create',
      consulSynchronizing: false,
      rules: {
        Name: [{ required: true, message: '集群名是必填项' }],
        Alias: [{ required: true, message: '可读别名是必填项' }],
        ShardTotal: [{ required: true, message: '分区总数是必填项' }]
      }
    }
  },
  created() {
    this.fetchClusterList()
  },
  methods: {
    fetchClusterList() {
      this.listLoading = true
      ListCluster(this.queryParams).then(response => {
        this.clusterList = response.Data.Items
        this.listTotal = response.Data.Total
        this.listLoading = false
      })
    },
    handleCreate() {
      this.temp = this.newClusterInfo()
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    handleUpdate(row) {
      this.temp.Id = row.Id
      this.temp.Name = row.Name
      this.temp.Alias = row.Alias
      this.temp.ShardTotal = row.ShardTotal
      this.temp.Desc = row.Desc
      this.dialogFormVisible = true
      this.dialogStatus = 'update'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    createCluster() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        AddCluster(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Created Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchClusterList()
        })
        this.dialogFormVisible = false
      })
    },
    updateCluster() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        UpdateCluster(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchClusterList()
        })
        this.dialogFormVisible = false
      })
    },
    synchronizeClusterInfoToConsul() {
      this.$confirm('确认要同步集群信息？', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      })
        .then(() => {
          this.consulSynchronizing = true
          SynchronizeClusterInfoToConsul()
            .then(response => {
              this.$notify({
                title: 'Success',
                message: 'Synchronize Successfully',
                type: 'success',
                duration: 2000
              })
            })
            .finally(() => {
              this.consulSynchronizing = false
            })
        })
        .catch(() => {})
    },
    handleCurrentChange(val) {
      this.queryParams.Page = val
      this.fetchClusterList()
    },
    newClusterInfo() {
      return {
        Id: 0,
        Name: '',
        Alias: '',
        ShardTotal: undefined,
        Desc: ''
      }
    }
  }
}
</script>
