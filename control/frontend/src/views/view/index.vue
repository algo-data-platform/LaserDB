<template>
  <div class="app-container">
    <el-table
      v-loading="listLoading"
      :data="list"
      element-loading-text="Loading"
      border
      fit
      highlight-current-row
    >
      <el-table-column align="center" label="ID" width="95">
        <template slot-scope="scope">
          {{ scope.row.Id }}
        </template>
      </el-table-column>
      <el-table-column label="名称">
        <template slot-scope="scope">
          {{ scope.row.Name }}
        </template>
      </el-table-column>
      <el-table-column label="描述">
        <template slot-scope="scope">
          <span>{{ scope.row.Desc }}</span>
        </template>
      </el-table-column>
      <el-table-column label="操作">
        <template slot-scope="scope">
          <el-button type="primary" size="mini" @click="viewFeature(scope.row)">
            查看
          </el-button>
          <el-button type="success" size="mini" @click="handleUpdate(scope.row)">
            修改
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    <el-dialog title="选择特征列表" :visible.sync="dialogVisible">
      <el-table
        :data="feature"
        element-loading-text="Loading"
        border
        fit
        highlight-current-row
      >
        <el-table-column align="center" label="ID" width="95">
          <template slot-scope="scope">
            {{ scope.row.Id }}
          </template>
        </el-table-column>
        <el-table-column label="名称">
          <template slot-scope="scope">
            {{ scope.row.Name }}
          </template>
        </el-table-column>
        <el-table-column label="描述">
          <template slot-scope="scope">
            <span>{{ scope.row.Desc }}</span>
          </template>
        </el-table-column>
        <el-table-column label="特征维度">
          <template slot-scope="scope">
            {{ scope.row.Dname }}
          </template>
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
      </el-table>
      <span slot="footer" class="dialog-footer">
        <el-button type="primary" @click="dialogVisible = false">确定</el-button>
      </span>
    </el-dialog>
  </div>
</template>

<script>
import { getList } from '@/api/view'

export default {
  data() {
    return {
      list: null,
      listLoading: true,
      dialogVisible: false,
      feature: null
    }
  },
  created() {
    this.fetchData()
  },
  methods: {
    fetchData() {
      this.listLoading = true
      getList().then(response => {
        this.list = response.Data.Items
        this.listLoading = false
      })
    },
    viewFeature(row) {
      this.feature = row.Features
      this.dialogVisible = true
    },
    handleUpdate(row) {
      this.$router.push({
        path: '/token/edit/' + row.Id
      })
    }
  }
}
</script>
