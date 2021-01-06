<template>
  <div class="app-container">
    <div class="filter-container">
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-edit"
        @click="handleCreate">
        添加
      </el-button>
      <el-checkbox v-model="showDesc" class="filter-item" style="margin-left:15px;" @change="tableKey=tableKey+1">
        描述
      </el-checkbox>
    </div>
    <el-table :key="tableKey" v-loading="listLoading" :data="groupList" element-loading-text="Loading" border fit
      highlight-current-row>
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="集群">
        <template slot-scope="scope">
          <span>{{ scope.row.ClusterName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="数据中心">
        <template slot-scope="scope">
          <span>{{ scope.row.DcName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="组名">
        <template slot-scope="scope">
          <span>{{ scope.row.Name }}</span>
        </template>
      </el-table-column>
      <el-table-column label="别名">
        <template slot-scope="scope">
          <span>{{ scope.row.Alias }}</span>
        </template>
      </el-table-column>
      <el-table-column label="活跃节点数">
        <template slot-scope="scope">
          <span>{{ scope.row.ActiveNumber }}</span>
        </template>
      </el-table-column>
      <el-table-column label="非活跃节点数">
        <template slot-scope="scope">
          <span>{{ scope.row.InactiveNumber }}</span>
        </template>
      </el-table-column>
      <el-table-column label="组内节点配置">
        <template slot-scope="scope">
          <span>{{ scope.row.NodeConfigName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="描述" v-if="showDesc">
        <template slot-scope="scope">
          <span>{{ scope.row.Desc }}</span>
        </template>
      </el-table-column>
      <el-table-column label="操作" align="center" width="220" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleUpdate(row)">修改</el-button>
          <el-button type="primary" size="mini" @click="deleteGroup(row)">删除</el-button>
          <el-button type="primary" size="mini" @click="handleReduceMetrics(row)">限流</el-button>
        </template>
      </el-table-column>
    </el-table>

    <div class="pagination-class">
      <el-pagination :background=true @current-change="handleCurrentChange" :current-page="queryParams.Page"
        :page-size="queryParams.Limit" layout="total, prev, pager, next, jumper" :total="listTotal">
      </el-pagination>
    </div>

    <el-dialog :title="textMap[dialogStatus]" :visible="dialogReduceFormVisible">
      <el-form ref="reduceDataForm" :rules="rules" :model="reduce" label-position="left" label-width="120px"
        style="width: 400px; margin-left:50px;">
        <el-form-item label="组名" prop="GroupName">
          <el-input v-model="reduce.GroupName" v-bind:disabled="dialogStatus=='reduce'" />
        </el-form-item>
        <el-form-item label="组内流量比例" prop="ReduceRate">
          <el-input v-model.number="reduce.ReduceRate" type="number" min="0" max="100"
            oninput="value=value.replace(/[^\d]/g,'')" placeholder="比例(0-100)"/>
        </el-form-item>
        <el-form-item label="模式" prop="ReduceMode">
           <el-select v-model="reduce.ReduceMode" placeholder="限制类型">
             <el-option v-for="(item,index) in ModeType" :key="index" :label="item.label" :value="item.value"> </el-option>
            </el-select>
        </el-form-item>
      </el-form>
      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogReduceFormVisible = false">
          取消
        </el-button>
        <el-button type="primary" @click="reduceMetricsGroup()">
          提交
        </el-button>
      </div>
      <el-table :key="tableKey" v-loading="shardListLoading" :data="shardList" :row-key="getRowKeys"
            :expand-row-keys="expands" element-loading-text="Loading" border fit highlight-current-row>
        <el-table-column align="center" label="ID" width="95" type="index"></el-table-column>
        <el-table-column label="节点 IP">
          <template slot-scope="scope">
            <span>{{ scope.row.Address }}</span>
          </template>
        </el-table-column>
        <el-table-column type="expand">
          <template scope="scope">
            <el-table class="reduce-table-expand" :data="scope.row.ShardInfo" border style="width: 100%">
              <el-table-column align="center" label="ID" width="95" type="index"></el-table-column>
              <el-table-column prop="ShardId" label="shardID">
                <template slot-scope="scope"><span>{{ scope.row.ShardId }}</span></template>
              </el-table-column>
              <el-table-column prop="scope" label="disable的Value">
                <template slot-scope="scope"><span>{{ scope.row.ReduceNum }}</span></template>
              </el-table-column>
            </el-table>
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>

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
        <el-form-item label="数据中心" prop="DcName">
          <el-select class="el-select" v-model="temp.DcName" :loading="listDcLoading" placeholder="请选择" 
            @visible-change="fetchDcData" v-bind:disabled="dialogStatus=='update'" @change="getDcIdByName(temp.DcName)">
            <el-option v-for="item in dcList" :key="item.Id" :label="item.Name" :value="item.Name"></el-option>
          </el-select>
        </el-form-item>
        <el-form-item label="组名" prop="Name">
          <el-input v-model="temp.Name" v-bind:disabled="dialogStatus=='update'" />
        </el-form-item>
        <el-form-item label="别名" prop="Alias">
          <el-input v-model="temp.Alias" />
        </el-form-item>
        <el-form-item label="组内节点配置" prop="NodeConfigName">
          <el-select class="el-select" v-model="temp.NodeConfigName" :loading="listNodeConfigLoading" placeholder="请选择"
            @visible-change="fetchNodeConfigData" @change="getNodeConfigIdByName(temp.NodeConfigName)">
            <el-option v-for="item in nodeConfigList" :key="item.Id" :label="item.Name" :value="item.Name">
            </el-option>
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
        <el-button type="primary" @click="dialogStatus==='create'? createGroup() : updateGroup()">
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
import { waves } from '@/directive/waves'
import { ListGroup, AddGroup, UpdateGroup, ReduceMetricsGroup, DeleteGroup } from '@/api/group'
import { NodeListConfig } from '@/api/node_config'
import { ListCluster } from '@/api/cluster'
import { ListDc } from '@/api/dc'
import {
  validGroupReduceRate
} from '@/utils/validate'

export default {
  directives: { waves },
  data() {
    return {
      listLoading: false,
      shardListLoading: false,
      listNodeConfigLoading: false,
      listClusterLoading: false,
      listDcLoading: false,
      groupList: undefined,
      shardList: undefined,
      groupDelete: {
        GroupId: undefined
      },
      nodeConfigList: undefined,
      clusterList: undefined,
      dcList: undefined,
      listTotal: undefined,
      tableKey: 0,
      showDesc: true,
      temp: this.newGroupInfo(),
      reduce: this.newGroupReduceInfo(),
      queryParams: {
        Page: 1,
        Limit: 20,
        ShowDetails: true
      },
      dialogFormVisible: false,
      dialogReduceFormVisible: false,
      textMap: {
        create: '创建组',
        update: '修改组',
        reduce: '限流设置'
      },
      ModeType:[{
        value:'read_kps_min_1',
        label:'读kps',
      },{
        value:'write_kps_min_1',
        label:'写kps'
      },{
        value:'read_bytes_min_1',
        label:'读byte',
      },{
        value:'write_bytes_min_1',
        label:'写byte',
      },{
        value:'live-sst-files-size',
        label:'shard大小'
      }],
      dialogStatus: 'create',
      rules: {
        Name: [{ required: true, message: '组名是必填项' }],
        DcId: [{ required: true, message: '数据中心必填项目' }],
        DcName: [{ required: true, message: '数据中心必填项目' }],
        Alias: [{ required: true, message: '可读别名是必填项' }],
        NodeConfigName: [{ required: true, message: '配置名称是必填项' }],
        ClusterName: [{ required: true, message: '集群名称是必填项' }],
        GroupName: [{ required: true, message: '组名是必填项' }],
        ReduceMode: [{ required: true, message: '限流类型是必填项' }],
        ReduceRate: [
          { required: true, message: '必填项不能为空', trigger: 'blur' },
          { type: 'number', message: '请输入数字格式' },
          { validator: validGroupReduceRate, trigger: 'blur' }
        ]
      },
      getRowKeys(row) {
        return row.id;
      },
      expands: []
    }
  },
  created() {
    this.fetchGroupList()
  },
  methods: {
    fetchGroupList() {
      this.listLoading = true
      ListGroup(this.queryParams).then(response => {
        this.groupList = response.Data.Items
        this.listTotal = response.Data.Total
        this.listLoading = false
      })
    },
    handleCreate() {
      this.temp = this.newGroupInfo()
      this.dialogFormVisible = true
      this.dialogStatus = 'create'
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate()
      })
    },
    handleUpdate(row) {
      this.temp.Id = row.Id;
      this.temp.Name = row.Name;
      this.temp.Alias = row.Alias;
      this.temp.NodeConfigId = row.NodeConfigId;
      this.temp.NodeConfigName = row.NodeConfigName;
      this.temp.Desc = row.Desc;
      this.temp.ClusterId = row.ClusterId;
      this.temp.ClusterName = row.ClusterName;
      this.temp.DcId = row.DcId;
      this.temp.DcName = row.DcName;
      this.dialogFormVisible = true;
      this.dialogStatus = 'update';
      this.$nextTick(() => {
        this.$refs['dataForm'].clearValidate();
      })
    },
    deleteGroup(row) {
      this.groupDelete.GroupId = row.Id
      this.$confirm('确定删除?', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(() => {
        DeleteGroup(this.groupDelete).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Delete Successfully',
            type: 'success',
            duration: 2000
          });
          this.fetchGroupList();
        });
      });
    },
    handleReduceMetrics(row) {
      this.reduce.GroupId = row.Id;
      this.reduce.GroupName = row.Name;
      this.dialogReduceFormVisible = true;
      this.dialogStatus = 'reduce';
      this.$nextTick(() => {
        this.$refs['reduceDataForm'].clearValidate();
      });
    },
    createGroup() {
      console.log(this.temp);
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        AddGroup(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Created Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchGroupList()
        })
        this.dialogFormVisible = false
      })
    },
    updateGroup() {
      this.$refs['dataForm'].validate(valid => {
        if (!valid) {
          return
        }

        UpdateGroup(this.temp).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchGroupList()
        })
        this.dialogFormVisible = false
      })
    },

    reduceMetricsGroup() {
      this.$refs['reduceDataForm'].validate(valid => {
        if (!valid) {
          return
        }
        this.shardListLoading = true
        ReduceMetricsGroup(this.reduce).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Update Successfully',
            type: 'success',
            duration: 2000
          })
          this.shardList = response.Data
          this.shardListLoading = false
          this.fetchGroupList()
        })
        this.dialogReduceFormVisible = false
      })
    },
    handleCurrentChange(val) {
      this.queryParams.Page = val;
      this.fetchGroupList();
    },
    fetchNodeConfigData(visibleFlag) {
      if (visibleFlag) {
        this.listNodeConfigLoading = true;
        NodeListConfig()
          .then(response => {
            this.nodeConfigList = response.Data.Items;
          })
          .finally(() => (this.listNodeConfigLoading = false))
      }
    },
    getNodeConfigIdByName(name) {
      for (let index = 0; index < this.nodeConfigList.length; ++index) {
        if (this.nodeConfigList[index].Name == name) {
          this.temp.NodeConfigId = this.nodeConfigList[index].Id;
          break;
        }
      }
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
    fetchDcData(visibleFlag) {
      if (visibleFlag) {
        this.listDcLoading = true;
        ListDc().then(response => {
          this.dcList = response.Data.Items;
        }).finally(() => {
          this.listDcLoading = false;
        });
      }
    },
    getClusterIdByName(name) {
      for (let index = 0; index < this.clusterList.length; ++index) {
        if (this.clusterList[index].Name == name) {
          this.temp.ClusterId = this.clusterList[index].Id;
          break;
        }
      }
    },
    getDcIdByName(name) { 
      for (let index = 0; index < this.dcList.length; ++index) {
        if (this.dcList[index].Name == name) {
          this.temp.DcId = this.dcList[index].Id;
          break;
        }
      }
    },
    newGroupInfo() {
      return {
        Id: 0,
        Name: '',
        Alias: '',
        DcId: undefined,
        DcName: '',
        NodeConfigId: undefined,
        NodeConfigName: '',
        ClusterId: undefined,
        ClusterName: '',
        Desc: ''
      }
    },
    newGroupReduceInfo() {
      return {
        GroupId: undefined,
        GroupName: '',
        ReduceRate: undefined,
        ReduceMode: ''
      }
    }
  }
}
</script>
