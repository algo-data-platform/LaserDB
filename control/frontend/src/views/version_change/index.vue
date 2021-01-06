<template>
  <div class="app-container">
    <div class="filter-container">
      <el-button
        class="filter-item"
        style="margin-left: 10px;"
        type="primary"
        icon="el-icon-edit"
        @click="createClear"
      >
        选择清库
      </el-button>

      <el-button
        class="filter-item"
        style="margin-left: 10px;"
        type="primary"
        icon="el-icon-edit"
        @click="createRollback"
      >
        选择回滚版本
      </el-button>
    </div>

    <el-table
      v-loading="listLoading"
      :data="list"
      element-loading-text="Loading"
      :default-sort = "{prop:'currentVersionTime',order:'descending'}"
      border
      fit
      highlight-current-row
    >
      <el-table-column align="center" label="ID" width="95" type="index"></el-table-column>
      <el-table-column label="Laser 数据库">
        <template slot-scope="scope">
          <span>{{ scope.row.databaseName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Laser 数据表">
        <template slot-scope="scope">
          <span>{{ scope.row.tableName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="当前版本">
        <template slot-scope="scope">
          <span>{{ scope.row.currentVersion }}</span>
        </template>
      </el-table-column>
      <el-table-column label="当前版本时间" prop="currentVersionTime" sortable>
        <template slot-scope="scope" >
          <span>{{ scope.row.currentVersionTime }}</span>
        </template>

      </el-table-column>
      <el-table-column label="操作" align="center" width="90" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="showDetail(row)">历史版本</el-button>
        </template>
      </el-table-column>
    </el-table>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="detailDialogFormVisible">
        <el-table
          :data="detailTemp.versionInfo"
          element-loading-text="Loading"
          border
          fit
          highlight-current-row
        >
        <el-table-column label="版本">
          <template slot-scope="scope">
            <span>{{ scope.row.versionName }}</span>
          </template>
        </el-table-column>
        <el-table-column label="操作时间">
          <template slot-scope="scope">
            <span>{{ scope.row.versionTime }}</span>
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="clearDialogFormVisible">
      <el-form
        ref="clearDataForm"
        :rules="clearRules"
        :model="formTemp"
        label-position="left"
        label-width="120px"
        style="width: 400px; margin-left:50px;"
      >
        <el-form-item
          label="Laser 数据库"
          prop="DatabaseName"
        >
          <el-select
            v-model="formTemp.DatabaseName"
            :loading="listDBLoading"
            placeholder="请选择"
            @visible-change="fetchDatabase"
            @change="getDatabaseIdByName(formTemp.DatabaseName)"
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
            v-model="formTemp.TableName"
            :loading="listDTLoading"
            placeholder="请选择"
            @visible-change="fetchDataTable"
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
      </el-form>
      <div
        slot="footer"
        class="dialog-footer"
      >
        <el-button @click="clearDialogFormVisible = false">
          取消
        </el-button>
        <el-button
          type="primary"
          @click="clearData()"
        >
          提交
        </el-button>
      </div>
    </el-dialog>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="rollbackDialogFormVisible">
      <el-form
        ref="rollbackDataForm"
        :rules="rollbackRules"
        :model="formTemp"
        label-position="left"
        label-width="120px"
        style="width: 400px; margin-left:50px;"
      >
        <el-form-item
          label="Laser 数据库"
          prop="DatabaseName"
        >
          <el-select
            v-model="formTemp.DatabaseName"
            :loading="listDBLoading"
            placeholder="请选择"
            @visible-change="fetchDatabase"
            @change="getDatabaseIdByName(formTemp.DatabaseName)"
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
            v-model="formTemp.TableName"
            :loading="listDTLoading"
            placeholder="请选择"
            @visible-change="fetchDataTable"
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
          label="Laser 数据版本"
          prop="VersionName"
        >
          <el-select
            v-model="formTemp.VersionName"
            :loading="listVSLoading"
            placeholder="请选择"
            @visible-change="fetchDataVersion"
          >
            <el-option
              v-for="item in dataVersionList"
              :key="item.versionName"
              :label="item.versionName"
              :value="item.versionName"
            >
            </el-option>
          </el-select>
        </el-form-item>
      </el-form>
      <div
        slot="footer"
        class="dialog-footer"
      >
        <el-button @click="rollbackDialogFormVisible = false">
          取消
        </el-button>
        <el-button
          type="primary"
          @click="rollbackData()"
        >
          提交
        </el-button>
      </div>
    </el-dialog>
  </div>
</template>

<script>
import {
  VersionChangeShow,
  VersionChangeClear,
  VersionChangeVersion,
  VersionChangeRollback
} from "@/api/version_change";
import { ListDatabase } from "@/api/database";
import { ListTable } from "@/api/table";
import { waves } from "@/directive/waves";
import { validCommandLimitValue } from "@/utils/validate";

export default {
  directives: { waves },
  data() {
    return {
      list: null,
      listLoading: true,
      detailDialogFormVisible: false,
      clearDialogFormVisible: false,
      rollbackDialogFormVisible: false,
      textMap: {
        detail: "数据版本详情",
        clear: "选择清理数据表",
        rollback: "选择数据表回滚版本",
      },
      dialogStatus: null,
      listDBLoading: true,
      listDTLoading: true,
      listVSLoading: true,
      databaseList: null,
      dataTableList: null,
      dataVersionList: null,
      detailTemp: {},
      formTemp: {},
      formQuery: {},
      clearRules: {
        DatabaseName: [{ required: true, message: "数据库名称是必填项" }],
        TableName: [{ required: true, message: "表名是必填项" }],
      },
      rollbackRules: {
        DatabaseName: [{ required: true, message: "数据库名称是必填项" }],
        TableName: [{ required: true, message: "表名是必填项" }],
        VersionName: [{ required: true, message: "表名是必填项" }],
      }
    };
  },
  created() {
    this.fetchData();
  },
  methods: {
    fetchData() {
      this.listLoading = true;
      VersionChangeShow().then(response => {
        this.list = response.Data;
        this.getDateByVersion(this.list);
        this.listLoading = false;
      });
    },
    showDetail(row) {
      console.log(row);
      this.detailTemp = row;
      this.detailDialogFormVisible = true;
      this.dialogStatus = "detail";
    },
   resetFormTemp() {
      this.formTemp = {};
    },
    createClear() {
      this.resetFormTemp();
      this.clearDialogFormVisible = true;
      this.dialogStatus = "clear";
      this.$nextTick(() => {
        this.$refs["clearDataForm"].clearValidate();
      });
    },
    clearData() {
      this.$refs["clearDataForm"].validate(valid => {
        if (!valid) {
          return;
        }
        VersionChangeClear(this.formTemp).then(response => {
          this.$notify({
            title: "Success",
            message: "清空数据表成功,生成新版本号" + response.Data,
            type: "success",
            duration: 3000
          });
          this.fetchData();
        });
        this.clearDialogFormVisible = false;
        this.$notify({
          title: "Success",
          message: "提交清空任务执行中...",
          type: "success",
          duration: 2000
        });
      });
    },
    createRollback() {
      this.resetFormTemp();
      this.rollbackDialogFormVisible = true;
      this.dialogStatus = "rollback";
        ListDatabase().then(response => {
          this.databaseList = response.Data.Items;
          this.listDBLoading = false;
        });
      this.$nextTick(() => {
        this.$refs["rollbackDataForm"].clearValidate();
      });
    },
    rollbackData() {
      this.$refs["rollbackDataForm"].validate(valid => {
        if (!valid) {
          return;
        }
        VersionChangeRollback(this.formTemp).then(response => {
          this.$notify({
            title: "Success",
            message: "回滚数据表成功" + response.Data,
            type: "success",
            duration: 3000
          });
          this.fetchData();
        });

        this.rollbackDialogFormVisible = false;

        this.$notify({
          title: "Success",
          message: "提交回滚任务执行中...",
          type: "success",
          duration: 2000
        });
      });
    },
    fetchDatabase() {
      this.listDBLoading = true
      ListDatabase().then(response => {
        this.databaseList = response.Data.Items
        this.listDBLoading = false
      });
    },
    getDatabaseIdByName(name) {
      for (let index = 0; index < this.databaseList.length; ++index) {
        if (name == this.databaseList[index].Name) {
          this.formTemp.DatabaseId = this.databaseList[index].Id;
          this.formQuery.DatabaseId = this.databaseList[index].Id;
          break;
        }
      }
    },
    fetchDataTable() {
      this.listDTLoading = true;
      if (!this.isEmpty(this.formQuery.DatabaseId)) {
        ListTable(this.formQuery).then(response => {
          this.dataTableList = response.Data.Items;
          this.listDTLoading = false;
        });
      }
    },
    fetchDataVersion() {
      this.listVSLoading = true;
      if (!this.isEmpty(this.formTemp.DatabaseName) && !this.isEmpty(this.formTemp.TableName)) {
        VersionChangeVersion(this.formTemp).then(response => {
          this.dataVersionList = response.Data;
          this.listVSLoading = false;
        });
      }
    },
    isEmpty(res){
      if(typeof res == "undefined" || res == null || res == ""){
          return true;
      }else{
          return false;
      }
    },
    getDateByVersion(data){
      var i = 0;
      var j = 0;
      for (i = 0; i < data.length; i++) {
        for (j = 0; j < data[i].versionInfo.length; j++) {
          if (data[i].versionInfo[j].versionName === data[i].currentVersion) {
            data[i].currentVersionTime = data[i].versionInfo[j].versionTime
            break
          }
        }
      }
    }
  }
}
</script>
