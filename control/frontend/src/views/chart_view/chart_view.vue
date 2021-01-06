<template>
  <div>
    <div v-bind:id="params.chartParams.title" style="width: 100%; height: 50vh"></div>
  </div>
</template>
 
<script>
import echarts from 'echarts'
export default {
  props: {
    dataSource: {
      type: Array,
      default() {
        return []
      }
    },
    params: {
      dataParams: {
        // 记录x轴数据的field名
        xField: {
          type: String
        },
        // 记录y轴数据的field名
        yField: {
          type: String
        },
        // param {}: record; return string: category描述
        getCategoryDesc: {
          type: Function
        }
      },
      chartParams: {
        title: {
          type: String
        },
        xAxis: {
          type: String
        },
        yAxis: {
          type: String
        },
        xType: {
          type: String
        },
        yType: {
          type: String
        }
      }
    },
    viewLoading: {
      type: Boolean
    }
  },
  mounted() {
    if (this.dataSource != undefined
          && this.viewLoading === false) {
      this.initAndDraw()
    }
  },
  data() {
    return {
      // 折线图句柄
      chart: undefined,
      // echart中dataSet的数据格式
      dataSet: [],
      // map(category描述，id)
      categoryIdMap: undefined
    }
  },
  methods: {
    /**
      * 整理为echarts支持的数组格式 [[x, y0, y1, ...], ...]
    */
    arrangeDataSet() {
      if (this.dataSource.length === 0) {
        return
      }

      // 整理后的数据[[x, y0, y1, ...], ...]
      var dataSet = []

      // 获取绘图需要的x和y数据
      let categoryIdMap = new Map()
      let yData = []
      let categorySerie = []
      this.dataSource.forEach(record => {
        let currentDesc = this.params.dataParams.getCategoryDesc(record)
        if (!categoryIdMap.has(currentDesc)) {
          categoryIdMap.set(currentDesc, categoryIdMap.size)
        }
        let categoryId = categoryIdMap.get(currentDesc)
        categorySerie.push({
          categoryId: categoryId,
          categoryDesc: currentDesc
        })
        dataSet.push([eval('record.' + this.params.dataParams.xField)])
        yData.push(eval('record.' + this.params.dataParams.yField))
      })

      // 将属于不同分类的数据拆到不同的列，空出来的位置用null填充
      let categoryNum = categoryIdMap.size
      let recordNum = this.dataSource.length
      for (let duplicateIndex = 0; duplicateIndex < categoryNum; duplicateIndex++) {
        for (let recordIndex = 0; recordIndex < recordNum; recordIndex++) {
          if (duplicateIndex == categorySerie[recordIndex].categoryId) {
            dataSet[recordIndex].push(yData[recordIndex])
          } else {
            dataSet[recordIndex].push(null)
          }
        }
      }


      this.dataSet = dataSet
      this.categoryIdMap = categoryIdMap
    },
    /**
     * 获取图形参数
    */
    getChartOption() {
      let chartLeft = '3%'
      let maxCategoryLen = 0
      for (var category of this.categoryIdMap) {
          if (category[0].length > maxCategoryLen) {
            maxCategoryLen = category[0].length
          }
      }
      let legendPercent = 2 + maxCategoryLen / 3
      let chartRight = legendPercent + '%'
      let legendLeft = (100 - legendPercent) + '%'

      // 设置折线图
      let gridOption = {
        right: chartRight,
        left:chartLeft
      }
  
      // 设置legend
      let legendOption = {
        orient: 'vertical',
        left: legendLeft,
        bottom: '50%'
      }

      // 设置title
      let titleOption = {
        textAlign: 'center',
        text: this.params.chartParams.title,
        left: '50%'
      }

      // 设置x,y轴
      let xAxisOption = {
        type: this.params.chartParams.xType,
        name: this.params.chartParams.xAxis
      }
      let yAxisOption = {
        type: this.params.chartParams.yType,
        name: this.params.chartParams.yAxis
      }

      // 设置数据
      let series = []
      for (let [ categoryDesc , categoryId] of this.categoryIdMap) {
        series.push({
          type: 'line',
          showSymbol: false,
          connectNulls: true,
          name: categoryDesc,
          encode: {
            x: 0,
            y: categoryId + 1
          }
        })
      }

      var option = {
        tooltip: {
          trigger: 'axis'
        },
        legend: legendOption,
        title: titleOption,
        grid: gridOption,
        xAxis: xAxisOption,
        dataZoom: [
          {
            type: 'inside',
            zoomOnMouseWheel: 'ctrl',
            moveOnMouseMove: 'ctrl'
          }
        ],
        yAxis: yAxisOption,
        dataset: {
          source: this.dataSet
        },
        series: series
      }
      return option
    },
    /**
     * 提示图形正在加载中
     */
    chartLoading() {
      if (this.chart != undefined) {
        echarts.dispose(this.chart)
      }
      this.chart = echarts.init(document.getElementById(this.params.chartParams.title))
      this.chart.showLoading({
        text: '数据正在努力加载...',
        textStyle: { fontSize : 30 , color: '#444' },
        effectOption: { backgroundColor: 'rgba(0, 0, 0, 0)' }
      })
    },
    /**
     * 数据加载完成，绘制折线图
     */
    drawChart() {
      if (this.dataSource === undefined || this.dataSource.length === 0) {
        return
      }

      this.arrangeDataSet()

      let chartOption = this.getChartOption()
      this.chart.setOption(chartOption)
    },
    /**
     * 所有数据创建时已就绪，初始化并立即绘制折线图
     */
    initAndDraw() {
      if (this.chart != undefined) {
        echarts.dispose(this.chart)
      }
      this.chart = echarts.init(document.getElementById(this.params.chartParams.title))
      this.drawChart()
    }
  },
  watch: {
    viewLoading(loading) {
      if (loading) {
        this.chartLoading()
      } else {
        this.chart.hideLoading()
        this.drawChart()
      }
    }
  }
}
</script>
