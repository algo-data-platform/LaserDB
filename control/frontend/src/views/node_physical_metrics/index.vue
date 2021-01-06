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
        <el-form label-position="left">
          <el-form-item label="节点地址:">
            <el-select class="el-select" v-model="selectedAddresses" multiple :loading="addressLoading" placeholder="节点地址"
              @visible-change="addressSelectBoundry" @change="handleSelectedAddressChange" >
              <el-option v-for="item in addressList" :key="item" :label="item" :value="item">
              </el-option>
            </el-select>
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
      <el-table-column label="节点地址">
        <template slot-scope="scope">
          <span>{{ scope.row.ServiceAddress }}</span>
        </template>
      </el-table-column>
      <el-table-column label="时间">
        <template slot-scope="scope">
          <span>{{ new Date(scope.row.CollectTime).format('yyyy-MM-dd hh:mm:ss') }}</span>
        </template>
      </el-table-column>
      <el-table-column label="CPU使用率(%)" v-if="tableShowColumnSet.has('CPU使用率(%)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.CpuUsageRate }}</span>
        </template>
      </el-table-column>
      <el-table-column label="CPU User(%)" v-if="tableShowColumnSet.has('CPU User(%)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.CpuUsageRateUser }}</span>
        </template>
      </el-table-column>
      <el-table-column label="CPU System(%)" v-if="tableShowColumnSet.has('CPU System(%)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.CpuUsageRateSystem }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Mem总量(GB)" v-if="tableShowColumnSet.has('Mem总量(GB)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.MemorySizeTotal }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Mem使用量(GB)" v-if="tableShowColumnSet.has('Mem使用量(GB)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.MemorySizeUsage }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Mem使用率(%)" v-if="tableShowColumnSet.has('Mem使用率(%)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.MemoryUsageRate }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Disk总量(GB)" v-if="tableShowColumnSet.has('Disk总量(GB)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.DiskSizeTotal }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Disk使用量(GB)" v-if="tableShowColumnSet.has('Disk使用量(GB)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.DiskSizeUsage }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Disk使用率(%)" v-if="tableShowColumnSet.has('Disk使用率(%)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.DiskUsageRate }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Net in(MB/S)" v-if="tableShowColumnSet.has('Net in(MB/S)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.NetworkIn }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Net out(MB/S)" v-if="tableShowColumnSet.has('Net out(MB/S)')" >
        <template slot-scope="scope">
          <span>{{ scope.row.NetworkOut }}</span>
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
      <el-tab-pane label="cpu使用情况" >
        <el-card class="box-card" v-if="chartTab === '0'" >
          <chartView :params="dataParams.get('CPU 使用率')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('CPU User使用率')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('CPU System使用率')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
        </el-card>
      </el-tab-pane>

      <el-tab-pane label="内存使用情况" >
        <el-card class="box-card" v-if="chartTab === '1'" >
          <chartView :params="dataParams.get('内存使用率')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('内存总空间')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('内存使用空间')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
        </el-card>
      </el-tab-pane>

      <el-tab-pane label="磁盘使用情况" >
        <el-card class="box-card" v-if="chartTab === '2'" >
          <chartView :params="dataParams.get('磁盘使用率')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('磁盘总空间')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('磁盘使用空间')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
        </el-card>
      </el-tab-pane>

      <el-tab-pane label="网络IO监控" >
        <el-card class="box-card" v-if="chartTab === '3'" >
          <chartView :params="dataParams.get('NetworkIn')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
          <chartView :params="dataParams.get('NetworkOut')" :viewLoading="metricDataLoading" :dataSource="metricDataAll" ></chartView>
        </el-card>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>
 
<script>
import chartView from '../chart_view/chart_view'
import { waves } from '@/directive/waves'
import { ListNodePhysicalMetrics, ListNodeList } from '@/api/node_physical_metrics'
import { getNDaysBefore } from '../../utils/common'
import { checkList } from '../view/checklist'
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
      addressLoading: true,
      addressList: [],
      selectedAddresses: [],
      isSelectingAddress: false,
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
        ServiceAddresses: [],
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
          'CPU 使用率',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'CpuUsageRate',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: 'CPU 使用率',
              xAxis: '时间',
              yAxis: 'CPU使用率(%)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          'CPU User使用率',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'CpuUsageRateUser',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: 'CPU User使用率',
              xAxis: '时间',
              yAxis: 'CPU User(%)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          'CPU System使用率',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'CpuUsageRateSystem',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: 'CPU System使用率',
              xAxis: '时间',
              yAxis: 'CPU System(%)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          '内存总空间',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'MemorySizeTotal',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '内存总空间',
              xAxis: '时间',
              yAxis: 'Mem总量(GB)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          '内存使用空间',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'MemorySizeUsage',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '内存使用空间',
              xAxis: '时间',
              yAxis: 'Mem使用量(GB)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          '内存使用率',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'MemoryUsageRate',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '内存使用率',
              xAxis: '时间',
              yAxis: 'Mem使用率(%)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          '磁盘总空间',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'DiskSizeTotal',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '磁盘总空间',
              xAxis: '时间',
              yAxis: 'Disk总量(GB)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          '磁盘使用空间',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'DiskSizeUsage',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '磁盘使用空间',
              xAxis: '时间',
              yAxis: 'Disk使用量(GB)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          '磁盘使用率',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'DiskUsageRate',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: '磁盘使用率',
              xAxis: '时间',
              yAxis: 'Disk使用率(%)',
              xType: 'time',
              yType: 'value'
            }
          }
        ],
        [
          'NetworkIn',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'NetworkIn',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: 'NetworkIn',
              xAxis: '时间',
              yAxis: 'Net in(MB/S)',
              xType: 'time',
              yType: 'value'
            }
        }],
        [
          'NetworkOut',
          {
            dataParams: {
              xField: 'CollectTime',
              yField: 'NetworkOut',
              getCategoryDesc: this.getCategoryDesc
            },
            chartParams: {
              title: 'NetworkOut',
              xAxis: '时间',
              yAxis: 'Net out(MB/S)',
              xType: 'time',
              yType: 'value'
            }
        }]
      ])
    }
  },
  created() {
    this.queryParams.StartTime = Date.parse(this.startTime) / 1000
    this.queryParams.EndTime = Date.parse(this.endTime) / 1000
    this.requestDataOfFirstNode()
  },
  mounted() {
    this.getColumeOptions()
  },
  methods: {
    requestDataOfFirstNode() {
      let queryNodesListParams = {
        StartTime: this.queryParams.StartTime,
        EndTime: this.queryParams.EndTime,
        TimeType: this.queryParams.TimeType
      }
      ListNodeList(queryNodesListParams).then(response => {
        if (response.Data != null && response.Data.length > 0) {
          this.selectedAddresses = [response.Data[0].ServiceAddress]
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
    addressSelectBoundry(begin) {
      if (begin) {
        this.isSelectingAddress = true
        this.fetchAllNodesAddress()
      } else {
        // 结束选择时，查询当前选择地址列表的信息
        this.isSelectingAddress = false
        this.handleSelect()
      }
    },
    // 开始选择地址时，根据其他选项获取当前支持的地址清单
    fetchAllNodesAddress() {
      let queryNodesListParams = {
        StartTime: this.queryParams.StartTime,
        EndTime: this.queryParams.EndTime,
        TimeType: this.queryParams.TimeType
      }
      this.addressLoading = true
      this.addressList = []
      ListNodeList(queryNodesListParams).then(response => {
        if (response.Data != null && response.Data.length > 0) {
          response.Data.forEach(metricRecord => {
            this.addressList.push(metricRecord.ServiceAddress)
          })
        }
        this.addressLoading = false
      })
    },
    // 每个筛选条件修改后，更新表格和折线图
    handleSelect() {
      // 更新表格请求参数
      this.queryParams.StartTime = Date.parse(this.startTime) / 1000
      this.queryParams.EndTime = Date.parse(this.endTime) / 1000
      // 由于数据量太大，若用户删除所有地址（默认查询所有地址的信息），则强制查询第一个地址的信息
      if (this.selectedAddresses.length === 0) {
        this.requestDataOfFirstNode()
        return
      }
      this.queryParams.ServiceAddresses = this.selectedAddresses
      this.queryParams.TimeType = this.timeType
      this.queryParams.Page = 1
      this.queryParams.Limit = 0

      // 更新表格和折线图
      this.updateTableAndCharts()
    },
    /**
     *  直接在主界面上点x删除节点时，不会触发visiable-change事件，而此时需要刷新页面
     *  该场景通过this.isSelectingAddress变量跟下拉选择的场景区分
     */
    handleSelectedAddressChange() {
      if (this.isSelectingAddress) {
        return
      }
      // 异步执行，metricDataLoading修改会触发界面更新，导致用户删除操作卡顿
      setTimeout(() => {
          this.metricDataLoading = true
          this.deleteDataOfRemovedAddress()
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
    deleteDataOfRemovedAddress() {
      let remainingAddressSet = new Set()
      this.selectedAddresses.forEach(address => {
        remainingAddressSet.add(address)
      })
      let strippedDataAll = []
      for (let index = 0; index < this.metricDataAll.length; index++) {
        if (remainingAddressSet.has(this.metricDataAll[index].ServiceAddress)) {
          strippedDataAll.push(this.metricDataAll[index])
        }
      }
      this.metricDataAll = strippedDataAll
    },
    getCategoryDesc(record) {
      return record.ServiceAddress
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
        if (this.metricDataAll[index].MemoryUsageRate === 0
             || this.metricDataAll[index].CpuUsageRate === 0
             || this.metricDataAll[index].CpuUsageRateUser === 0
             || this.metricDataAll[index].DiskUsageRate === 0
             || this.metricDataAll[index].NetworkIn === 0
             || this.metricDataAll[index].NetworkOut === 0) {
          this.metricDataAll.splice(index, 1)
        } else {
          index++
        }
      }
    },
    dataUnitConversion() {
      const GB_DIV = 1024 * 1024 * 1024
      const MB_DIV = 1024 * 1024
      for (let index = 0; index < this.metricDataAll.length; index++) {
        this.metricDataAll[index].CpuUsageRate = (this.metricDataAll[index].CpuUsageRate).toFixed(2)
        this.metricDataAll[index].CpuUsageRateUser = (this.metricDataAll[index].CpuUsageRateUser).toFixed(2)
        this.metricDataAll[index].CpuUsageRateSystem = (this.metricDataAll[index].CpuUsageRateSystem).toFixed(2)

        this.metricDataAll[index].MemorySizeTotal = Math.round(this.metricDataAll[index].MemorySizeTotal / GB_DIV)
        this.metricDataAll[index].MemorySizeUsage = Math.round(this.metricDataAll[index].MemorySizeUsage / GB_DIV)
        this.metricDataAll[index].MemoryUsageRate = (this.metricDataAll[index].MemoryUsageRate).toFixed(2)

        this.metricDataAll[index].DiskSizeTotal = Math.round(this.metricDataAll[index].DiskSizeTotal / GB_DIV)
        this.metricDataAll[index].DiskSizeUsage = Math.round(this.metricDataAll[index].DiskSizeUsage / GB_DIV)
        this.metricDataAll[index].DiskUsageRate = (this.metricDataAll[index].DiskUsageRate).toFixed(2)

        this.metricDataAll[index].NetworkIn = (this.metricDataAll[index].NetworkIn / MB_DIV).toFixed(2)
        this.metricDataAll[index].NetworkOut = (this.metricDataAll[index].NetworkOut / MB_DIV).toFixed(2)
      }
    },
    updateTableAndCharts() {
      this.metricDataLoading = true
      ListNodePhysicalMetrics(this.queryParams).then(response => {
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
