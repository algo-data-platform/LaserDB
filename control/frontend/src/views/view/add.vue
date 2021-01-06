<template>
  <div class="app-container">
    <el-form ref="form" :model="form" label-width="120px">
      <el-form-item label="名称">
        <el-input v-model="form.Name" />
      </el-form-item>
      <el-form-item label="特征选择">
        <template>
          <el-tabs :tab-position="tabPosition" style="height: 500px; overflow: scroll">
            <el-tab-pane :key="index" :label="item.Name" v-for="(item, index) in dims">
              <tab-pane :index="index" :did="item.Id" :init-selected="initSelectedFeatures" @updateData="updateData"/>
            </el-tab-pane>
          </el-tabs>
        </template>
      </el-form-item>
      <el-form-item label="描述">
        <el-input v-model="form.Desc" type="textarea" />
      </el-form-item>
      <el-form-item>
        <el-button type="primary" @click="onSubmit">保存</el-button>
        <el-button @click="onCancel">取消</el-button>
      </el-form-item>
    </el-form>
  </div>
</template>

<script>

import { getList } from '@/api/service'
import { AddView, UpdateView, getInfo } from '@/api/view'
import tabPane from './components/TabPane'

export default {
  components: {
    tabPane
  },
  data() {
    return {
      form: {
        Name: '',
        Desc: ''
      },
      tabPosition: 'left',
      dims: null,
      group: 'select_feature',
      isEdit: false,
      id: undefined,
      initSelectedFeatures: []
    }
  },
  created() {
    this.id = this.$route.params.id
    if (this.id !== undefined) {
      this.isEdit = true
    }
    this.fetchData()
  },
  methods: {
    onSubmit() {
      const data = {
        Name: this.form.Name,
        Desc: this.form.Desc,
        Fids: [],
        Id: parseInt(this.id)
      }
      Object.keys(this.dims).forEach(index => {
        if (this.dims[index].selected !== undefined) {
          this.dims[index].selected.forEach(feature => {
            data.Fids.push(feature.Id)
          })
        }
      })
      const submit = (this.isEdit) ? UpdateView : AddView
      submit(data).then(response => {
        this.$notify({
          title: 'Success',
          message: 'Created Successfully',
          type: 'success',
          duration: 2000
        })
        this.$router.push({ name: 'view_index' })
      })
    },
    onCancel() {
      this.$message({
        message: 'cancel!',
        type: 'warning'
      })
    },
    fetchData() {
      getList({ Status: 1 }).then(response => {
        this.dims = response.Data.Items
      })
      if (this.isEdit) {
        getInfo(this.id).then(response => {
          const view = response.Data
          this.form.Desc = view.Desc
          this.form.Name = view.Name
          view.Features.forEach(feature => {
            this.initSelectedFeatures.push(feature.Fid)
          })
        })
      }
    },
    updateData(index, unselected, selected) {
      this.dims[index].unselected = unselected
      this.dims[index].selected = selected
      console.info(this.dims)
    }
  }
}
</script>

<style scoped>
.line{
  text-align: center;
}
</style>
