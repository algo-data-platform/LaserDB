<template>
  <div>
    <el-cascader class="el-select" v-model="internalSelectedMachines" :show-all-levels="false" placeholder="请选择"
      :options="machineOptions" :props="cascaderProps" :loading="listMachineLoading" @change="onSelectionChange"
      @visible-change="handleMachineCascaderVisibleChange">
    </el-cascader>
  </div>
</template>
<script>
import { ListMachineCategory } from '@/api/machine_category'

export default {
  name: 'MachineSelection',
  props: {
    selectedMachines: {},
    multipleSelection: {
      type: Boolean,
      default: true
    }
  },
  data() {
    return {
      cascaderProps: {
        multiple: this.multipleSelection,
        emitPath: false
      },
      machineOptions: undefined,
      internalSelectedMachines: this.selectedMachines,
      listMachineLoading: false
    }
  },
  watch: {
    selectedMachines(val) {
      this.internalSelectedMachines = val
    }
  },
  created() {
    this.fetchMachineOptions()
  },
  methods: {
    fetchMachineOptions() {
      this.listMachineLoading = true
      ListMachineCategory()
        .then(response => {
          this.machineOptions = []
          const machineCategories = response.Data.Items
          machineCategories.forEach(category => {
            const categoryOption = {
              value: category.Id,
              label: category.Name,
              children: []
            }
            category.Machines.forEach(machine => {
              const machineOption = {
                value: machine.Ip,
                label: machine.Ip
              }
              categoryOption.children.push(machineOption)
            })
            if (categoryOption.children.length > 0) {
              this.machineOptions.push(categoryOption)
            }
          })
        })
        .finally(() => {
          this.listMachineLoading = false
        })
    },
    handleMachineCascaderVisibleChange(visible) {
      if (visible) {
        this.fetchMachineOptions()
      }
    },
    onSelectionChange(val) {
      this.$emit('update:selectedMachines', this.internalSelectedMachines)
    }
  }
}
</script>
