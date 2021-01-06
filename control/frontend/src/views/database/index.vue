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
      <el-checkbox
        v-model="showDesc"
        class="filter-item"
        style="margin-left:15px;"
        @change="databaseKey=databaseKey+1"
      >
        描述
      </el-checkbox>
    </div>
    <el-table
      :key="databaseKey"
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
          <span>{{ scope.row.Name }}</span>
        </template>
      </el-table-column>
      <el-table-column
        label="描述"
        v-if="showDesc"
      >
        <template slot-scope="scope">
          <span>{{ scope.row.Desc }}</span>
        </template>
      </el-table-column>
      <el-table-column label="操作" align="center" width="150" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleUpdate(row)">修改</el-button>
          <el-button type="primary" size="mini" @click="deleteDatabase(row)">删除</el-button>
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
          prop="Name"
        >
          <el-input
            v-model="temp.Name"
            v-bind:disabled="dialogStatus==='update'"
          />
        </el-form-item>
        <el-form-item
          label="描述"
          prop="Desc"
        >
          <el-input
            v-model="temp.Desc"
            type="textarea"
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
import { ListDatabase, AddDatabase, UpdateDatabase, DeleteDatabase } from "@/api/database";
import { waves } from "@/directive/waves";

export default {
  directives: { waves },
  data() {
    return {
      list: null,
      databaseDelete: {
        DatabaseId: undefined
      },
      listTotal: undefined,
      listLoading: true,
      databaseKey: 0,
      showDesc: true,
      listQuery: {
        Page: 1,
        Limit: 20
      },
      dialogFormVisible: false,
      textMap: {
        create: "创建数据库",
        update: "修改数据库"
      },
      dialogStatus: "create",
      temp: {},
      rules: {
        Name: [{ required: true, message: "数据库名称是必填项" }]
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
      ListDatabase(this.listQuery).then(response => {
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
        Desc: "",
        Name: ""
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

        AddDatabase(this.temp).then(response => {
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
    deleteDatabase(row) {
      this.databaseDelete.DatabaseId = row.Id
      this.$confirm('确定删除?', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(() => {
        DeleteDatabase(this.databaseDelete).then(response => {
          this.$notify({
            title: 'Success',
            message: 'Delete Successfully',
            type: 'success',
            duration: 2000
          })
          this.fetchData()
        })
      })
    },
    updateData() {
      this.$refs["dataForm"].validate(valid => {
        if (!valid) {
          return;
        }

        UpdateDatabase(this.temp).then(response => {
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
    handleCurrentChange(val) {
      this.listQuery.Page = val;
      this.fetchData();
    }
  }
};
</script>
