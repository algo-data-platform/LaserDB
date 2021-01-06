<template>
  <div>
    <el-select class="el-select" v-model="internalAnsibleConfigId" :loading="listAnsibleConfigLoading" placeholder="请选择"
      @visible-change="fetchAnsibleConfigList" @change="onConfigIdChange">
      <el-option v-for="item in ansibleConfigList" :key="item.Id" :label="item.Name" :value="item.Id">
      </el-option>
    </el-select>
  </div>
</template>


<script>
import { ListAnsibleConfig } from '@/api/ansible_config'

export default {
  name: 'AnsbileConfigSelection',
  props: {
    ansibleConfigId: {
      type: Number,
      default: undefined
    }
  },
  watch: {
    ansibleConfigId(val) {
      this.internalAnsibleConfigId = val
    }
  },
  data() {
    return {
      ansibleConfigList: undefined,
      internalAnsibleConfigId: this.ansibleConfigId,
      listAnsibleConfigLoading: false
    }
  },
  created() {
    this.fetchAnsibleConfigList()
  },
  methods: {
    fetchAnsibleConfigList() {
      this.listAnsibleConfigLoading = true
      ListAnsibleConfig()
        .then(response => {
          this.ansibleConfigList = response.Data.Items
        })
        .finally(() => {
          this.listAnsibleConfigLoading = false
        })
    },
    onConfigIdChange(val) {
      console.log(val)
      this.$emit('update:ansibleConfigId', this.internalAnsibleConfigId)
    }
  }
}
</script>