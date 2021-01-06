<template>
  <div v-if="showTip">
    <el-alert :center="true" :title="tipText" type="warning" show-icon :closable="false" />
  </div>
</template>

<script>
import { checkUnsynchronizedConfig } from '@/api/config_validator'

export default {
  name: 'UnsynchronizedConfigTip',
  data() {
    return {
      ConfigType: {
        ClusterConfig: 1,
        TableConfig: 2
      },
      showTip: false,
      tipText: ''
    }
  },
  created() {
    this.checkUnsynchronizedConfig()
  },
  methods: {
    checkUnsynchronizedConfig() {
      checkUnsynchronizedConfig().then(response => {
        const types = Object.keys(response.Data.UnsynchronizedConfigTypes)
        if (types.length !== 0) {
          this.showTip = true
          this.tipText = '有变更的'
          let hasFirst = false
          types.forEach(item => {
            if (hasFirst) {
              this.tipText += '和'
            }
            if (parseInt(item) === this.ConfigType.ClusterConfig) {
              this.tipText += '集群配置信息'
              hasFirst = true
            } else if (parseInt(item) === this.ConfigType.TableConfig) {
              this.tipText += '表配置信息'
              hasFirst = true
            }
          })
          this.tipText += '没有同步'
        } else {
          this.showTip = false
        }
      })
      setTimeout(this.checkUnsynchronizedConfig, 5000)
    }
  }
}
</script>
