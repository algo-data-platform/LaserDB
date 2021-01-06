<template>
  <div class="app-container">
    <el-form>
    <el-row :gutter="0">
      <el-col :span="4">
        <div class="block">
          <span class="demonstration"></span>
          <el-form-item label="开始时间">
          <el-date-picker v-model="startTime" align="left" type="date" placeholder="开始日期" @change="handleSelect" :clearable="false" > </el-date-picker>
          </el-form-item>
        </div>
      </el-col>
      <el-col :span="1">
        <div type="primary" icon="el-icon-minus"></div>
      </el-col>
      <el-col :span="4">
        <div class="block">
          <span class="demonstration"></span>
          <el-form-item label="结束时间">
          <el-date-picker v-model="endTime" align="left" type="date" placeholder="结束日期" @change="handleSelect" :clearable="false" > </el-date-picker>
          </el-form-item>
        </div>
      </el-col>
    </el-row>
    </el-form>
    <el-table v-loading="metricDataLoading" :data="metricDataCurPage" element-loading-text="Loading" border fit
      highlight-current-row >
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="时间">
        <template slot-scope="scope">
          <span>{{ new Date(scope.row.CollectTime).format('yyyy-MM-dd hh:mm:ss') }}</span>
        </template>
      </el-table-column>
      <el-table-column label="IO放大指数">
        <template slot-scope="scope">
          <span>{{ scope.row.IoRatePerMb }}</span>
        </template>
      </el-table-column>
      <el-table-column label="网络放大指数">
        <template slot-scope="scope">
          <span>{{ scope.row.NetAmplificationRatio }}</span>
        </template>
      </el-table-column>
      <el-table-column label="磁盘放大指数">
        <template slot-scope="scope">
          <span>{{ scope.row.DiskAmplificationRatio }}</span>
        </template>
      </el-table-column>
    </el-table>

    <div class="pagination-class">
      <el-pagination :background=true @current-change="handleCurrentChange" :current-page="queryParams.Page"
        :page-size="PageLimit" layout="total, prev, pager, next, jumper" :total="listTotal">
      </el-pagination>
    </div>

    <br> </br>
    <el-tabs type="border-card" v-model="chartTab" >
      <el-tab-pane label="IO放大指数" >
        <br> </br>
        <el-card class="box-card">
          <chartView :params="dataParams.get('IO放大指数')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" v-if="chartTab === '0'" ></chartView>
        </el-card>
      </el-tab-pane>
      <el-tab-pane label="网络放大指数">
        <el-card class="box-card">
          <chartView :params="dataParams.get('网络放大指数')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" v-if="chartTab === '1'" ></chartView>
        </el-card>
      </el-tab-pane>
      <el-tab-pane label="磁盘放大指数">
        <el-card class="box-card">
          <chartView :params="dataParams.get('磁盘放大指数')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" v-if="chartTab === '2'" ></chartView>
        </el-card>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>
 
<script>
import chartView from '../chart_view/chart_view'
import { waves } from '@/directive/waves'
import { ListSystemIndices } from '@/api/system_indices'
import { getNDaysBefore } from '../../utils/common'
export default {
  directives: { waves },
  components: {
    chartView
  },
  data() {
    return {
      // 查询参数
      endTime: new Date(),
      startTime: getNDaysBefore(7), // 默认展示当前时间往前7天的数据
      PageLimit: 10,
      queryParams: {
        Page: 1,
        Limit: this.PageLimit,
        StartTime: 0,
        EndTime: 0
      },
      // 查询结果
      metricDataAll: undefined,
      metricDataLoading: false,
      metricDataCurPage: undefined,
      // 列表显示参数
      listTotal: undefined,
      // 绘图相关数据
      chartTab: '0',
      dataParams: new Map([
        [
          'IO放大指数', 
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'IoRatePerMb',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: 'IO放大指数',
              xAxis: '时间',
              yAxis: '放大指数(倍数)',
              xType: 'time',
              yType: 'value',
            },
          }
        ],
        [
          '网络放大指数', 
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'NetAmplificationRatio',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '网络放大指数',
              xAxis: '时间',
              yAxis: '放大指数(倍数)',
              xType: 'time',
              yType: 'value',
            },
          }
        ],
        [
          '磁盘放大指数', 
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'DiskAmplificationRatio',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '磁盘放大指数',
              xAxis: '时间',
              yAxis: '放大指数(倍数)',
              xType: 'time',
              yType: 'value',
            },
          }
        ]
      ])
    }
  },
  mounted() {
    this.queryParams.StartTime = Date.parse(this.startTime) / 1000
    this.queryParams.EndTime = Date.parse(this.endTime) / 1000
    this.queryParams.Limit = 0
    this.updateTableAndCharts()
  },
  methods: {
    // 获取表格相关数据
    updateTableInfo() {
      this.metricDataCurPage = []
      this.listTotal = this.metricDataAll.length
      let startIndex = (this.queryParams.Page - 1) * this.PageLimit
      let endIndex = startIndex + this.PageLimit
      for (var recordCount = startIndex; recordCount <  endIndex && recordCount < this.listTotal; recordCount++) {
        this.metricDataCurPage.push(this.metricDataAll[recordCount])
      }
    },
    // 表格选择页
    handleCurrentChange(val) {
      this.queryParams.Page = val
      this.queryParams.Limit = this.PageLimit
      this.updateTableInfo()
    },
    // 每个筛选条件修改后，更新表格和折线图
    handleSelect() {
      // 更新表格请求参数
      this.queryParams.StartTime = Date.parse(this.startTime) / 1000
      this.queryParams.EndTime = Date.parse(this.endTime) / 1000
      this.queryParams.Page = 1
      this.queryParams.Limit = 0

      // 更新表格和折线图
      this.updateTableAndCharts()
    },
    getCategoryDesc(record) {
      return ''
    },
    clearAllData() {
      this.metricDataAll = []
      this.metricDataCurPage = []
      this.listTotal = 0
    },
    removeInvalidDataLocal() {
      for (let index = 0; ;) {
        if (index >= this.metricDataAll.length) {
          break
        }
        if (this.metricDataAll[index].IoRatePerMb === 0
             || this.metricDataAll[index].NetAmplificationRatio === 0
             || this.metricDataAll[index].DiskAmplificationRatio === 0) {
          this.metricDataAll.splice(index, 1)
        } else {
          index++
        }
      }
    },
    dataUnitConversion() {
      const MB_DIV = 1024 * 1024
      for (let index = 0; index < this.metricDataAll.length; index++) {
      }
    },
    updateTableAndCharts() {
      this.metricDataLoading = true
      ListSystemIndices(this.queryParams).then(response => {
        // 符合筛选结果的所有信息
        this.metricDataAll = response.Data.Items

        // 删除无效信息
        this.removeInvalidDataLocal()

        // 请求到数据为空时，清除数据
        if (this.metricDataAll.length === 0) {
          this.clearAllData()
          this.metricDataLoading = false
          return
        }

        // 修改数据单位
        this.dataUnitConversion()

        // 更新表格第一页的信息
        this.updateTableInfo()
        
        this.metricDataLoading = false
      })
    }
  }
}
</script>
