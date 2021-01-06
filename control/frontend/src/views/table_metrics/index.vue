<template>
  <div class="app-container">
    <el-form>
    <el-row :gutter="0">
      <el-col :span="5">
        <div class="block">
          <span class="demonstration"></span>
          <el-form-item label="开始日期:">
          <el-date-picker v-model="startTime" align="left" type="date" placeholder="开始日期" @change="handleSelect" :clearable="false" > </el-date-picker>
          </el-form-item>
        </div>
      </el-col>
      <el-col :span="1">
        <div type="primary" icon="el-icon-minus"></div>
      </el-col>
      <el-col :span="5">
        <div class="block">
          <span class="demonstration"></span>
          <el-form-item label="结束日期:">
          <el-date-picker v-model="endTime" align="left" type="date" placeholder="结束日期" @change="handleSelect" :clearable="false" > </el-date-picker>
          </el-form-item>
        </div>
      </el-col>
      <el-col :span="5">
        <el-form>
          <el-form-item label="数据表:">
            <el-cascader v-model="selectedTables" :options="tableOption" :props="cascaderProps" :loading="tableLoading" placeholder="数据表"
              @visible-change="tableSelectBoundry" @change="handleSelectedTableChange" >
            </el-cascader>
          </el-form-item>
        </el-form>
      </el-col>
      <el-col :span="5">
        <el-form-item label="时间类型:">
        <el-select class="el-select" v-model="timeType" placeholder="时间类型"
            @change="handleSelect" >
            <el-option v-for="item in timeTypeList" :key="item.value" :label="item.desc" :value="item.value">
            </el-option>
        </el-select>
        </el-form-item>
      </el-col>
    </el-row>
    </el-form>
    <checklist :buttonTip="buttonTip" :options="checkColumnOptions" @checkChange="handleColumnChange" ></checklist>
    <el-table v-loading="metricDataLoading" :data="metricDataCurPage" element-loading-text="Loading" border fit
      highlight-current-row >
      <el-table-column align="center" label="ID" width="95" type="index">
      </el-table-column>
      <el-table-column label="数据表">
        <template slot-scope="scope">
          <span>{{ scope.row.DatabaseName + ":" + scope.row.TableName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="数据量(GB)" v-if="tableShowColumnSet.has('数据量(GB)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.DataSize }}</span>
        </template>
      </el-table-column>
      <el-table-column label="KPS 总(W)" v-if="tableShowColumnSet.has('KPS 总(W)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.Kps }}</span>
        </template>
      </el-table-column>
      <el-table-column label="写 KPS(W)" v-if="tableShowColumnSet.has('写 KPS(W)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.KpsWrite }}</span>
        </template>
      </el-table-column>
      <el-table-column label="读 KPS(W)" v-if="tableShowColumnSet.has('读 KPS(W)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.KpsRead }}</span>
        </template>
      </el-table-column>
      <el-table-column label="BPS 总(MB)" v-if="tableShowColumnSet.has('BPS 总(MB)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.Bps }}</span>
        </template>
      </el-table-column>
      <el-table-column label="写 BPS(MB)" v-if="tableShowColumnSet.has('写 BPS(MB)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.BpsWrite }}</span>
        </template>
      </el-table-column>
      <el-table-column label="读 BPS(MB)" v-if="tableShowColumnSet.has('读 BPS(MB)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.BpsRead }}</span>
        </template>
      </el-table-column>
      <el-table-column label="分区数" v-if="tableShowColumnSet.has('分区数')" >
        <template slot-scope="scope">
          <span>{{ scope.row.PartitionNumber }}</span>
        </template>
      </el-table-column>
      <el-table-column label="时间" >
        <template slot-scope="scope">
          <span>{{ new Date(scope.row.CollectTime).format('yyyy-MM-dd hh:mm:ss') }}</span>
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
      <el-tab-pane label="数据量" >
        <el-card class="box-card" v-if="chartTab === '0'" >
          <chartView :params="dataParams.get('数据量')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
        </el-card>
      </el-tab-pane>
    

      <el-tab-pane label="KPS" >
        <el-card class="box-card" v-if="chartTab === '1'" >
          <chartView :params="dataParams.get('总 KPS')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('写 KPS')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('读 KPS')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
        </el-card>
      </el-tab-pane>
    

      <el-tab-pane label="BPS" >
        <el-card class="box-card" v-if="chartTab === '2'" >
          <chartView :params="dataParams.get('总 BPS')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('写 BPS')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('读 BPS')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
        </el-card>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>
 
<script>
import chartView from '../chart_view/chart_view'
import { waves } from '@/directive/waves'
import { ListTableRunningMetrics, ListTableList } from '@/api/table_metrics'
import { getNDaysBefore } from '../../utils/common'
import Checklist from '../view/checklist.vue'
export default {
  directives: { waves },
  components: {
    chartView,
    Checklist
  },
  data() {
    return {
      // 查询参数
      endTime: new Date(),
      startTime: getNDaysBefore(7), // 默认展示当前时间往前7天的数据
      cascaderProps: { multiple: true },
      tableLoading: true,
      selectedTables: [],
      tableOption: [],
      isSelectingTable: false,
      timeTypeList: [
        {
          value : 1,
          desc: "平峰",
        },
        {
          value : 2,
          desc: "高峰",
        }
      ],
      timeType: 2,
      PageLimit: 10,
      queryParams: {
        Page: 1,
        Limit: this.PageLimit,
        StartTime: 0,
        EndTime: 0,
        Tables: [],
        TimeType: 0
      },
      // 查询结果
      metricDataAll: undefined,
      metricDataLoading: false,
      metricDataCurPage: undefined,
      listTotal: undefined,
      // 列筛选相关参数
      buttonTip: "选择显示的列",
      checkColumnOptions: [],
      tableShowColumnSet: new Set(),
      // 绘图相关数据
      chartTab: '0',
      dataParams: new Map([
        [
          '数据量',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'DataSize',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '数据量',
              xAxis: '时间',
              yAxis: '数据量(GB)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          '分区数',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'PartitionNumber',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '分区数',
              xAxis: '时间',
              yAxis: '分区数',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          '总 KPS',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'Kps',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '总 KPS',
              xAxis: '时间',
              yAxis: 'KPS 总(W)',
              xType: 'time',
              yType: 'value',
            },
          }
        ],
        [
          '写 KPS',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'KpsWrite',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '写 KPS',
              xAxis: '时间',
              yAxis: '写 KPS(W)',
              xType: 'time',
              yType: 'value',
            },
          }
        ],
        [
          '读 KPS',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'KpsRead',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '读 KPS',
              xAxis: '时间',
              yAxis: '读 KPS(W)',
              xType: 'time',
              yType: 'value',
            },
          }
        ],
        [
          '总 BPS',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'Bps',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '总 BPS',
              xAxis: '时间',
              yAxis: 'BPS 总(MB)',
              xType: 'time',
              yType: 'value',
            },
          }
        ],
        [
          '写 BPS',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'BpsWrite',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '写 BPS',
              xAxis: '时间',
              yAxis: '写 BPS(MB)',
              xType: 'time',
              yType: 'value'
            },
          }
        ],
        [
          '读 BPS',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'BpsRead',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '读 BPS',
              xAxis: '时间',
              yAxis: '读 BPS(MB)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
      ])
    }
  },
  created() {
    this.queryParams.StartTime = Date.parse(this.startTime) / 1000
    this.queryParams.EndTime = Date.parse(this.endTime) / 1000
    this.requestDataOfFirstTable()
  },
  mounted() {
    this.getColumeOptions()
  },
  methods: {
    arrangeTableOptions(tableList) {
      this.tableOption = []
      let databases = []
      tableList.forEach(table => {
        databases.push(table.DatabaseName)
      })
      databases = Array.from(new Set(databases))

      databases.forEach(database => {
        let databaseOption = {
          value: database,
          label: database,
          children: []
        }
        this.tableOption.push(databaseOption)
      })

      tableList.forEach(table => {
        for (let i = 0; i < this.tableOption.length; i++) {
          if (table.DatabaseName === this.tableOption[i].value) {
            let tableItemOption = {
              value: table.TableName,
              label: table.TableName,
            }
            this.tableOption[i].children.push(tableItemOption)
          }
        }
      })
    },
    requestDataOfFirstTable() {
      let queryTablesListParams = {
        StartTime: this.queryParams.StartTime,
        EndTime: this.queryParams.EndTime,
        TimeType: this.queryParams.TimeType
      }
      ListTableList(queryTablesListParams).then(response => {
        if (response.Data != null && response.Data.length > 0) {
          this.arrangeTableOptions(response.Data)
          let firstTable = response.Data[0]
          this.selectedTables.push([firstTable.DatabaseName, firstTable.TableName])
          this.handleSelect()
        }
      })
    },
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
    // 开始或结束节点选择
    tableSelectBoundry(begin) {
      if (begin) {
        this.isSelectingTable = true
        this.fetchAllTables()
      } else {
        // 结束选择时，查询当前选择地址列表的信息
        this.isSelectingTable = false
        this.handleSelect()
      }
    },
    // 开始选择地址时，根据其他选项获取当前支持的地址清单
    fetchAllTables() {
      let queryTablesListParams = {
        StartTime: this.queryParams.StartTime,
        EndTime: this.queryParams.EndTime,
        TimeType: this.queryParams.TimeType
      }
      this.tableLoading = true
      ListTableList(queryTablesListParams).then(response => {
        this.arrangeTableOptions(response.Data)
      })
    },
    // 每个筛选条件修改后，更新表格和折线图
    handleSelect() {
      // 更新表格请求参数
      this.queryParams.StartTime = Date.parse(this.startTime) / 1000
      this.queryParams.EndTime = Date.parse(this.endTime) / 1000
      // 由于数据量太大，若用户删除所有地址（默认查询所有地址的信息），则强制查询第一个地址的信息
      if (this.selectedTables.length === 0) {
        this.requestDataOfFirstTable()
        return
      }
      let tempTables = []
      this.selectedTables.forEach(table => {
        tempTables.push({
          DatabaseName: table[0],
          TableName: table[1]
        })
      })
      this.queryParams.Tables = tempTables
      this.queryParams.TimeType = this.timeType
      this.queryParams.Page = 1
      this.queryParams.Limit = 0

      // 更新表格和折线图
      this.updateTableAndCharts()
    },
    /**
     *  直接在主界面上点x删除节点时，不会触发visiable-change事件，而此时需要刷新页面
     *  该场景通过this.isSelectingTable变量跟下拉选择的场景区分
     */
    handleSelectedTableChange() {
      if (this.isSelectingTable) {
        return
      }
      // 异步执行，metricDataLoading修改会触发界面更新，导致用户删除操作卡顿
      setTimeout(() => {
          this.metricDataLoading = true
          this.deleteDataOfRemovedTable()
          // 更新表格第一页的信息
          this.queryParams.Page = 1
          this.queryParams.Limit = this.PageLimit
          this.updateTableInfo()
          // metricDataLoading修改太快，不会被子组件抓到，延迟1ms修改
          setTimeout(() => {
              this.metricDataLoading = false
            },
            1
          )
        },
        1
      )
    },
    deleteDataOfRemovedTable() {
      let remainingTableSet = new Set()
      this.selectedTables.forEach(table => {
        remainingTableSet.add(table[0] + ":" + table[1])
      })
      let strippedDataAll = []
      for (let index = 0; index < this.metricDataAll.length; index++) {
        let table = this.metricDataAll[index].DatabaseName + ":" + this.metricDataAll[index].TableName
        if (remainingTableSet.has(table)) {
          strippedDataAll.push(this.metricDataAll[index])
        }
      }
      this.metricDataAll = strippedDataAll
    },
    getCategoryDesc(record) {
      return record.DatabaseName + ":" + record.TableName
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
        if (this.metricDataAll[index].DataSize === 0
              || this.metricDataAll[index].PartitionNumber === 0) {
          this.metricDataAll.splice(index, 1)
        } else {
          index++
        }
      }
    },
    dataUnitConversion() {
      const GB_DIV = 1024 * 1024 * 1024
      const MB_DIV = 1024 * 1024
      const W_DIV = 10000
      for (let index = 0; index < this.metricDataAll.length; index++) {
        this.metricDataAll[index].DataSize = Math.round(this.metricDataAll[index].DataSize * 1024 / GB_DIV)

        this.metricDataAll[index].Kps = (this.metricDataAll[index].Kps / W_DIV).toFixed(2)
        this.metricDataAll[index].KpsWrite = (this.metricDataAll[index].KpsWrite / W_DIV).toFixed(2)
        this.metricDataAll[index].KpsRead = (this.metricDataAll[index].KpsRead / W_DIV).toFixed(2)

        this.metricDataAll[index].Bps = (this.metricDataAll[index].Bps / MB_DIV).toFixed(2)
        this.metricDataAll[index].BpsWrite = (this.metricDataAll[index].BpsWrite / MB_DIV).toFixed(2)
        this.metricDataAll[index].BpsRead = (this.metricDataAll[index].BpsRead / MB_DIV).toFixed(2)

      }
    },
    updateTableAndCharts() {
      this.metricDataLoading = true
      ListTableRunningMetrics(this.queryParams).then(response => {
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
    },
    getColumeOptions() {
      this.checkColumnOptions = []
      this.dataParams.forEach(dataParam => {
        this.checkColumnOptions.push(dataParam.chartParams.yAxis)
      })
    },
    handleColumnChange(value) {
      this.tableShowColumnSet = value
    }
  }
}
</script>
