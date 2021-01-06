<template>
  <div >
    <el-popover placement="right" title="请选择" width="200" trigger="click" >
      <el-button slot="reference" type="primary" >{{buttonTip}}</el-button>
      <el-checkbox :indeterminate="isIndeterminate" v-model="checkAll" @change="handleCheckAllChange">全选</el-checkbox>
      <el-checkbox-group v-model="selectedItems" @change="handleCheckChange">
        <el-checkbox v-for="item in options" :label="item" :key="item">{{item}}</el-checkbox>
      </el-checkbox-group>
    </el-popover>
  </div>
</template>
<script>
export default {
  props: {
    buttonTip: {
      type: String,
      default() {
        return ""
      }
    },
    options: {
      type: Array,
      default() {
        return []
      }
    },
  },
  data() {
    return {
      isIndeterminate: true,
      checkAll: true,
      selectedItems: []
    }
  },
  methods: {
    handleCheckChange(value) {
      let selectedSet = new Set()
      this.selectedItems.forEach(item => {
        selectedSet.add(item)
      })
      let checkedCount = value.length;
      this.checkAll = checkedCount === this.options.length;
      this.isIndeterminate = checkedCount > 0 && checkedCount < this.options.length;
      this.$emit('checkChange', selectedSet)
    },
    handleCheckAllChange(val) {
      this.selectedItems = val ? this.options : [];
      this.isIndeterminate = false;
      this.handleCheckChange(this.selectedItems)
    }
  },
  watch: {
    options(value) {
      this.selectedItems = this.options
      this.handleCheckChange(this.selectedItems)
    }
  }
}
</script>
