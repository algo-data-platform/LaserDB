<template>
  <div class="app-container">
    <div class="filter-container">
      <el-button
        class="filter-item"
        style="margin-left: 10px;"
        type="primary"
        icon="el-icon-edit"
        @click="handleCreate"
      >
        添加
      </el-button>
      <el-button
        class="filter-item"
        style="margin-left: 10px;"
        type="primary"
        icon="el-icon-upload"
        :loading="consulSynchronizing"
        @click="synchronizeDataToConsul"
      >
        同步
      </el-button>
    </div>
    <el-table
      v-loading="listLoading"
      :data="list"
      element-loading-text="Loading"
      border
      fit
      highlight-current-row
    >
      <el-table-column
        align="center"
        label="ID"
        width="95"
        type="index"
      >
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
      <el-table-column label="读超时">
        <template slot-scope="scope">
          <span>{{ scope.row.ReadTimeout }}</span>
        </template>
      </el-table-column>
      <el-table-column label="写超时">
        <template slot-scope="scope">
          <span>{{ scope.row.WriteTimeout }}</span>
        </template>
      </el-table-column>
      <el-table-column label="流量允许阈值">
        <template slot-scope="scope">
          <span>{{ scope.row.AllowedFlow }}</span>
        </template>
      </el-table-column>
      <el-table-column
        label="操作"
        align="center"
        width="80"
        class-name="small-padding fixed-width"
      >
        <template slot-scope="{row}">
          <el-button
            type="primary"
            size="mini"
            @click="handleUpdate(row)"
          >
            修改
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    <div class="pagination-class">
      <el-pagination
        :background=true
        @current-change="handleCurrentChange"
        :current-page="listQuery.Page"
        :page-size="listQuery.Limit"
        layout="total, prev, pager, next, jumper"
        :total="listTotal"
      >
      </el-pagination>
    </div>
    <el-dialog
      :title="textMap[dialogStatus]"
      :visible.sync="dialogFormVisible"
    >
      <el-form
        ref="dataForm"
        :rules="rules"
        :model="temp"
        label-position="left"
        label-width="120px"
        style="width: 400px; margin-left:50px;"
      >
        <el-form-item
          label="Laser 数据库"
          prop="DatabaseName"
        >
          <el-select
            v-model="temp.DatabaseName"
            :loading="listDBLoading"
            placeholder="请选择"
            @visible-change="fetchDatabase"
            @change="getDatabaseIdByName(temp.DatabaseName)"
            v-bind:disabled="dialogStatus=='update'"
          >
            <el-option
              v-for="item in databaseList"
              :key="item.Id"
              :label="item.Name"
              :value="item.Name"
            >
            </el-option>
          </el-select>
        </el-form-item>
        <el-form-item
          label="Laser 数据表"
          prop="TableName"
        >
          <el-select
            v-model="temp.TableName"
            :loading="listDTLoading"
            placeholder="请选择"
            @visible-change="fetchDataTable"
            v-bind:disabled="dialogStatus=='update'"
          >
            <el-option
              v-for="item in dataTableList"
              :key="item.Id"
              :label="item.Name"
              :value="item.Name"
            >
            </el-option>
          </el-select>
        </el-form-item>
        <el-form-item
          label="读超时"
          prop="ReadTimeout"
        >
          <el-input
            v-model.number="temp.ReadTimeout"
            type="number"
            oninput="value=value.replace(/[^\d]/g,'')"
          />
        </el-form-item>
        <el-form-item
          label="写超时"
          prop="WriteTimeout"
        >
          <el-input
            v-model.number="temp.WriteTimeout"
            type="number"
            oninput="value=value.replace(/[^\d]/g,'')"
          />
        </el-form-item>
        <el-form-item
          label="流量允许阈值(0-100)"
          prop="AllowedFlow"
        >
          <el-input
            v-model.number="temp.AllowedFlow"
            type="number"
            oninput="value=value.replace(/[^\d]/g,'')"
          />
        </el-form-item>
      </el-form>
      <div
        slot="footer"
        class="dialog-footer"
      >
        <el-button @click="dialogFormVisible = false">
          取消
        </el-button>
        <el-button
          type="primary"
          @click="dialogStatus==='create'? createData() : updateData()"
        >
          提交
        </el-button>
      </div>
    </el-dialog>
  </div>
</template>

<script>
import {
  ProxyListTableConfig,
  ProxyAddTableConfig,
  ProxyUpdateTableConfig,
  ProxySynchronizeDataToConsul
} from "@/api/proxy_config";
import { ListDatabase } from "@/api/database";
import { ListTable } from "@/api/table";
import { waves } from "@/directive/waves";
import { validCommandLimitValue } from '@/utils/validate'

export default {
  directives: { waves },
  data() {
    return {
      list: null,
      listTotal: undefined,
      currentPage: 1,
      listLoading: true,
      listQuery: {
        Page: 1,
        Limit: 20,
        DatabaseId : undefined
      },
      dialogFormVisible: false,
      textMap: {
        create: "创建数据表配置",
        update: "修改数据表配置"
      },
      dialogStatus: "create",
      listDBLoading: true,
      listDTLoading: true,
      databaseList: null,
      dataTableList: null,
      consulSynchronizing: false,
      temp: {},
      rules: {
        DatabaseName: [{ required: true, message: "数据库名称是必填项" }],
        TableName: [{ required: true, message: "表名是必填项" }],
        ReadTimeout: [
          { required: true, message: "读超时时长是必填项", trigger: 'blur' },
          { type: "number", message: "请输入数字格式" }
        ],
        WriteTimeout: [
          { required: true, message: "写超时时长是必填项", trigger: 'blur' },
          { type: "number", message: "请输入数字格式" }
        ],
        AllowedFlow: [
          { required: true, message: "流量允许阈值是必填项，范围0-100", trigger: 'blur' },
          { type: "number", message: "请输入数字格式" },
          { validator: validCommandLimitValue, trigger: 'blur' }
        ],
      }
    };
  },
  created() {
    this.fetchData();
    this.resetTemp();
  },
  methods: {
    fetchData() {
      this.listLoading = true;
      ProxyListTableConfig(this.listQuery).then(response => {
        this.list = response.Data.Items;
        this.listTotal = response.Data.Total;
        this.listLoading = false;
      });
    },
    handleFilter() {
      this.listQuery.Page = 1;
      this.fetchData();
    },
    resetTemp() {
      this.temp = {
        Id: undefined,
        TableName: "",
        DatabaseId: undefined,
        ReadTimeout: undefined,
        WriteTimeout: undefined,
        AllowedFlow: undefined,
        DatabaseName: "",
      };
    },
    handleCreate() {
      this.resetTemp();
      this.dialogFormVisible = true;
      this.dialogStatus = "create";
      this.$nextTick(() => {
        this.$refs["dataForm"].clearValidate();
      });
    },
    createData() {
      this.$refs["dataForm"].validate(valid => {
        if (!valid) {
          return;
        }

        ProxyAddTableConfig(this.temp).then(response => {
          this.$notify({
            title: "Success",
            message: "Created Successfully",
            type: "success",
            duration: 2000
          });
          this.fetchData();
        });
        this.dialogFormVisible = false;
      });
    },
    handleUpdate(row) {
      this.temp = row;
      this.dialogFormVisible = true;
      this.dialogStatus = "update";
      this.$nextTick(() => {
        this.$refs["dataForm"].clearValidate();
      });
    },
    updateData() {
      this.$refs["dataForm"].validate(valid => {
        if (!valid) {
          return;
        }

        ProxyUpdateTableConfig(this.temp).then(response => {
          this.$notify({
            title: "Success",
            message: "Update Successfully",
            type: "success",
            duration: 2000
          });
          this.fetchData();
        });
        this.dialogFormVisible = false;
      });
    },
    synchronizeDataToConsul() {
      this.consulSynchronizing = true;
      ProxySynchronizeDataToConsul()
        .then(reponse => {
          this.$notify({
            title: "Success",
            message: "Synchronize Successfully",
            type: "success",
            duration: 2000
          });
          this.consulSynchronizing = false;
        })
        .catch(e => {
          this.consulSynchronizing = false;
        });
    },
    fetchDatabase(visibleFlag) {
      if (visibleFlag) {
        this.listDBLoading = true;
        ListDatabase().then(response => {
          this.databaseList = response.Data.Items;
          this.listDBLoading = false;
        });
      }
    },
    getDatabaseIdByName(name) {
      for (let index = 0; index < this.databaseList.length; ++index) {
        if (name == this.databaseList[index].Name) {
          this.temp.DatabaseId = this.databaseList[index].Id;
          this.listQuery.DatabaseId = this.databaseList[index].Id;
          break;
        }
      }
    },
    fetchDataTable() {
      this.listDTLoading = true;
      ListTable(this.listQuery).then(response => {
        this.dataTableList = response.Data.Items;
        this.listDTLoading = false;
      });
    },
    handleCurrentChange(val) {
      this.listQuery.Page = val;
      this.fetchData();
    }
  }
};
</script>
