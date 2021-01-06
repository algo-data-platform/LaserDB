<template>
  <div class="app-container">
    <el-form
      ref="dataForm"
      :rules="rulesDB"
      :model="temp"
      label-position="left"
      label-width="120px"
      style="width: 400px; margin-left:50px;">
      <el-form-item
        label="Laser 数据库"
        prop="DatabaseName">
        <el-select
          v-model="temp.DatabaseName"
          :loading="listDBLoading"
          placeholder="请选择"
          style="width: 300px;"
          @visible-change="fetchDatabase"
          @change="InitInputDatabaseByName(temp.DatabaseName)">
          <el-option
            v-for="item in databaseList"
            :key="item.Id"
            :label="item.Name"
            :value="item.Name">
          </el-option>
        </el-select>
      </el-form-item>
      <el-form-item
        label="Laser 数据表"
        prop="DataTableName">
        <el-select
          v-model="temp.DataTableName"
          :loading="listDTLoading"
          placeholder="请选择"
          style="width: 300px;"
          @visible-change="fetchDataTable"
          @change="InitInputDataTableByName(temp.DataTableName)">
          <el-option
            v-for="item in dataTableList"
            :key="item.Id"
            :label="item.Name"
            :value="item.Name">
          </el-option>
        </el-select>
      </el-form-item>
    </el-form>
    <el-form
      ref="dataFormKey"
      :rules="rulesKey"
      :model="inputInfo"
      label-position="left"
      label-width="120px"
      style="width: 420px; margin-left:50px;">
      <el-form-item
        label="数据 key 名"
        prop="keyName[0]">
        <el-input
          placeholder="data key name"
          v-model="inputInfo.keyName[0]"
          type="text">
        </el-input>
      </el-form-item>
    </el-form><br><br><br>
    是否对整集群 check：
    <el-checkbox
      v-model="inputInfo.Checkall"
      class="filter-item"
      style="">
    </el-checkbox><br><br><br>
    <el-button
      class="filter-item"
      style="width: 100px; margin-left:500px;"
      type="primary"
      @click="listData">
      sumbit
    </el-button><br><br><br>
    <el-table
      :key="keycheck"
      v-loading="listLoading"
      :data="IdInfo"
      element-loading-text="Loading"
      border
      fit
      highlight-current-row>
      <el-table-column align="center" label="ShardId">
        <template slot-scope="scope">
          <span>{{ scope.row.ShardId }}</span>
        </template>
      </el-table-column>
      <el-table-column align="center" label="PartitionId">
        <template slot-scope="scope">
          <span>{{ scope.row.PartitionId }}</span>
        </template>
      </el-table-column>
    </el-table>
    <el-table
      :key="keycheck"
      v-loading="listLoading"
      :data="ValueInfo"
      element-loading-text="Loading"
      border
      fit
      highlight-current-row>
      <el-table-column align="center" label="ID" width="120" type="index"></el-table-column>
      <el-table-column align="center" label="Role">
        <template slot-scope="scope">
          <span>{{ scope.row.Role }}</span>
        </template>
      </el-table-column>
      <el-table-column align="center" label="Address">
        <template slot-scope="scope">
          <span>{{ scope.row.Ip }}:{{ scope.row.Port }}</span>
        </template>
      </el-table-column>
      <el-table-column align="center" label="Dc">
        <template slot-scope="scope">
          <span>{{ scope.row.Dc }}</span>
        </template>
      </el-table-column>
      <el-table-column align="center" label="check status">
        <template slot-scope="scope">
          <span>{{ scope.row.Status }}</span>
        </template>
      </el-table-column>
      <el-table-column align="center" label="value info">
        <template slot-scope="scope">
          <span>{{ scope.row.Value }}</span>
        </template>
      </el-table-column>

    </el-table>
  </div>
</template>

<script>
import { ListTable } from "@/api/table";
import { ListDatabase } from "@/api/database";
import { ListKeycheck } from "@/api/keycheck";
import { waves } from "@/directive/waves";

export default {
  directives: { waves },
  data() {
    return {
      IdInfo : {
        PartitionId : [],
        ShardId : [],
      },
      ValueInfo: {
        Role : [],
        Ip : [],
        Port : [],
        Status : [],
        Value : [],
      },
      ValueNum: undefined,
      listLoading : false,
      inputInfo: {
        dbName : "",
        tbName : "",
        keyName : [],
        Checkall : false,
      },
      fetchDTName :{
        Page : undefined,
        Limit : undefined,
        DatabaseId : undefined,
      },
      temp: {},
        listDBLoading: true,
        listDTLoading: true,
        databaseList: null,
        dataTableList: null,
      rulesDB: {
        DatabaseName: [{required: true, message: "数据库名称是必填项"}],
        DataTableName: [{required: true, message: "数据表名称是必填项"}]
      },
      rulesKey: {
        keyName: [{required: true, message: "数据 key 名称是必填项"}]
      }
    };
  },
   methods: {
     resetTemp() {
       this.temp = {
         DatabaseId: undefined,
         DataTableId: undefined,
         DatabaseName: "",
         DataTableName: "",
       };
     },
     fetchDatabase() {
       this.listDBLoading = true;
       ListDatabase().then(response => {
         this.databaseList = response.Data.Items;
         this.listDBLoading = false;
         this.temp.DataTableName = "";
         this.inputInfo.keyName[0] = "";
        });
      },
     fetchDataTable() {
       this.listDTLoading = true;
       ListTable(this.fetchDTName).then(response => {
         this.dataTableList = response.Data.Items;
         this.listDTLoading = false;
       });
     },
     getDatabaseIdByName(name) {
       for (let index = 0; index < this.databaseList.length; ++index) {
         if (name == this.databaseList[index].Name) {
           this.fetchDTName.DatabaseId = this.databaseList[index].Id;
           break;
         }
       }
     },
     InitInputDatabaseByName(name) {
       this.inputInfo.dbName = name;
       this.getDatabaseIdByName(name);
     },
     InitInputDataTableByName(name) {
       this.inputInfo.tbName = name;
     },
     listData() {
       this.$refs["dataForm"].validate(valid => {
         if (!valid) {
           return;
         }

         this.listLoading = true;
         ListKeycheck(this.inputInfo).then(response => {
           this.IdInfo = response.Data.IdInfo;
           this.ValueInfo = response.Data.Items;
           this.listLoading = false;
         });
       });
     },
   }
};
</script>
