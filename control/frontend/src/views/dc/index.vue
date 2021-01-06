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
    <el-table :key="tableKey" v-loading="listLoading" :data="dcList" element-loading-text="Loading" border fit highlight-current-row>
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="集群">
        <template slot-scope="scope">
          <span>{{ scope.row.ClusterName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="数据中心名称">
        <template slot-scope="scope">
          <span>{{ scope.row.Name }}</span>
        </template>
      </el-table-column>
      <el-table-column label="分片总数">
        <template slot-scope="scope">
          <span>{{ scope.row.ShardNumber}}</span>
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

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogFormVisible" :close-on-click-modal="false">
      <el-form ref="dataForm" :rules="rules" :model="temp" label-position="left" label-width="120px"
        style="width: 400px; margin-left:50px;">
        <el-form-item label="所属集群" prop="ClusterName">
          <el-select class="el-select" v-model="temp.ClusterName" :loading="listClusterLoading" placeholder="请选择"
            @visible-change="fetchClusterData" @change="getClusterIdByName(temp.ClusterName)"
            v-bind:disabled="dialogStatus=='update'">
            <el-option v-for="item in clusterList" :key="item.Id" :label="item.Name" :value="item.Name">
            </el-option>
          </el-select>
        </el-form-item>
        <el-form-item label="数据中心名" prop="Name">
          <el-input v-model="temp.Name" v-bind:disabled="dialogStatus=='update'" />
        </el-form-item>
        <el-form-item label="分片总数" prop="ShardNumber">
          <el-input v-model.number="temp.ShardNumber" type="number" min="1" oninput="value=value.replace(/[^\d]/g,'')"
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
        <el-button type="primary" @click="dialogStatus==='create'? createDc() : updateDc()">
          提交
        </el-button>
      </div>
    </el-dialog>

  </div>
</template>
 
<script>
import { waves } from '@/directive/waves'
import {
  ListDc,
  AddDc,
  UpdateDc
} from '@/api/dc'
import { ListCluster } from '@/api/cluster'

export default {
  directives: { waves },
  data() {
    return {
      listLoading: false,
      listClusterLoading: false,
      dcList: undefined,
      listTotal: undefined,
      tableKey: 0,
      showDesc: true,
      temp: this.newDcInfo(),
      queryParams: {
        Page: 1,
        Limit: 20
      },
      dialogFormVisible: false,
      textMap: {
        create: '创建数据中心',
        update: '修改数据中心'
      },
      dialogStatus: 'create',
      consulSynchronizing: false,
      rules: {
        ClusterName: [{ required: true, message: '集群名称是必填项' }],
        Name: [
          { type: 'string', required: true, message: '数据中心名是必填项' },
          { validator: (rule, value, callback) => {
            if (!/^[a-zA-Z0-9-_]+$/.test(value)) {
              return callback(new Error('名字不能为除了a-zA-Z0-9-_字符以外的其他字符'));
            } else {
              return callback();
            }
          }, trigger: 'blur' }
        ],
        ShardNumber: [
          { type: 'integer', required: true, message: '数据中心分区总数是必填项' },
        ]
      }
    }
  },
  created() {
    this.fetchDcList()
  },
  methods: {
    fetchDcList() {
      this.listLoading = true
      ListDc(this.queryParams).then(response => {
        this.dcList = response.Data.Items
        this.listTotal = response.Data.Total
        this.listLoading = false
      })
    },
    handleCreate() {
      this.temp = this.newDcInfo()
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    handleUpdate(row) {
      this.temp.Id = row.Id;
      this.temp.Name = row.Name;
      this.temp.ShardNumber = row.ShardNumber;
      this.temp.ClusterId = row.ClusterId;
      this.temp.ClusterName = row.ClusterName;
      this.temp.Desc = row.Desc;
      this.dialogFormVisible = true;
      this.dialogStatus = 'update';
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate();
      });
    },
    fetchClusterData(visibleFlag) {
      if (visibleFlag) {
        this.listClusterLoading = true;
        ListCluster()
          .then(response => {
            this.clusterList = response.Data.Items;
          })
          .finally(() => (this.listClusterLoading = false));
      }
    },
    createDc() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        AddDc(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Created Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchDcList()
        })
        this.dialogFormVisible = false
      })
    },
    updateDc() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        UpdateDc(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchDcList()
        })
        this.dialogFormVisible = false
      })
    },
    handleCurrentChange(val) {
      this.queryParams.Page = val
      this.fetchDcList()
    },
    getClusterIdByName(name) {
      for (let index = 0; index < this.clusterList.length; ++index) {
        if (this.clusterList[index].Name == name) {
          this.temp.ClusterId = this.clusterList[index].Id;
          break;
        }
      }
    },
    newDcInfo() {
      return {
        Id: 0,
        ClusterId: undefined,
        ClusterName: '',
        Name: '',
        ShardNumber: undefined,
        Desc: ''
      }
    }
  }
}
</script>
