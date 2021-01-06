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
    <el-table :key="tableKey" v-loading="listLoading" :data="configList" element-loading-text="Loading" border fit
      highlight-current-row>
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="配置名称">
        <template slot-scope="scope">
          <span>{{ scope.row.Name }}</span>
        </template>
      </el-table-column>
      <el-table-column label="版本">
        <template slot-scope="scope">
          <span>{{ scope.row.Version }}</span>
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
          <el-button type="primary" size="mini" @click="deleteTableConfig(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <div class="pagination-class">
      <el-pagination :background=true @current-change="handleCurrentChange" :current-page="queryParams.Page"
        :page-size="queryParams.Limit" layout="total, prev, pager, next, jumper" :total="listTotal">
      </el-pagination>
    </div>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogFormVisible" width="80%">
      <TableConfigTemplate ref="tableConfigCmp" :isNameSelected="true" :asTemplate="false"
        :configName="editingConfig.Name">
      </TableConfigTemplate>
    </el-dialog>
  </div>
</template>


<script>
import {
  ListTableConfig,
  AddTableConfig,
  UpdateTableConfig,
  DeleteTableConfig,
  ListTableConfigWithDetailItems
} from "@/api/table_config";
import { waves } from "@/directive/waves";
import { validPartitionNumber } from "@/utils/validate";
import TableConfigTemplate from "@/views/table_config_template/index.vue";

export default {
  directives: { waves },
  components: { TableConfigTemplate },
  data() {
    return {
      listLoading: false,
      configList: undefined,
      listTotal: undefined,
      tableConfigDelete: {
        TableConfigId: undefined
      },
      tableKey: 0,
      showDesc: true,
      editingConfig: this.newConfigBasic(),
      queryParams: {
        Page: 1,
        Limit: 20,
        ExcludedNames: []
      },
      dialogFormVisible: false,
      textMap: {
        create: "创建表配置",
        update: "修改表配置"
      },
      dialogStatus: "create",
      boolText: {
        0: "否",
        1: "是"
      }
    };
  },
  created() {
    this.fetchConfigList();
  },
  methods: {
    fetchConfigList() {
      this.listLoading = true;
      this.queryParams.ExcludedNames.push("default");
      ListTableConfig(this.queryParams).then(response => {
        this.configList = response.Data.Items;
        this.listTotal = response.Data.Total;
        this.listLoading = false;
      });
    },
    handleCreate() {
      this.editingConfig = this.newConfigBasic();
      this.dialogFormVisible = true;
      this.dialogStatus = "create";
    },
    handleUpdate(row) {
      this.editingConfig = row;
      this.dialogFormVisible = true;
      this.dialogStatus = "update";
    },
    deleteTableConfig(row) {
      this.tableConfigDelete.TableConfigId = row.Id
      this.$confirm('确定删除?', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(() => {
        DeleteTableConfig(this.tableConfigDelete).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Delete Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchConfigList()
        })
      })
    },
    handleCurrentChange(val) {
      this.queryParams.Page = val;
      this.fetchConfigList();
    },
    postSaved() {
      this.dialogFormVisible = false;
      this.fetchConfigList();
    },
    newConfigBasic() {
      return {
        Id: 0,
        Name: "",
        Version: 0,
        IsDefault: 0,
        Desc: ""
      };
    }
  }
};
</script>
