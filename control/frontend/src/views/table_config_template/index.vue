<template>
  <div class="app-container">
    <div class="title-basic-info" v-if="asTemplate == false">
      <h3>基本信息</h3>
      <el-divider></el-divider>
    </div>
    <div class="basic-info" v-if="asTemplate == false">
      <el-form :ref="formRefs[0]" :model="tableConfigForm" style="margin-left:80px;" label-position="left"
        label-width="80px" size="mini" :rules="rules">
        <el-row :gutter="5">
          <el-col :span="22">
            <el-form-item label="配置名称" prop="Name">
              <el-input v-model="tableConfigForm.Name" placeholder="请输入配置名称"></el-input>
            </el-form-item>
          </el-col>
        </el-row>
        <el-row :gutter="5">
          <el-col :span="22">
            <el-form-item label="描述" prop="Desc">
              <el-input v-model="tableConfigForm.Desc" type="textarea"></el-input>
            </el-form-item>
          </el-col>
        </el-row>
      </el-form>
    </div>

    <div class="title-db-options">
      <h3>DbOptions</h3>
      <el-divider></el-divider>
    </div>
    <div class="db-options">
      <el-form :ref="formRefs[1]" :model="tableConfigForm" style="margin-left:80px;" label-position="left"
        label-width="80px" size="mini">
        <el-row :gutter="5" v-for="(item, index) in tableConfigForm.dbOptionsItems" :key="index">
          <el-col :span="11">
            <el-form-item :label="'配置项' + (index + 1)" :prop="'dbOptionsItems.' + index + '.Name'"
              :rules="rules.dbOptionsItemsName">
              <el-select v-if="isNameSelected" v-model="item.Name" filterable allow-create style="width:100%"
                placeholer="请选择配置项名称">
                <el-option v-for="templateItem in cachedDefaultConfigForm.dbOptionsItems" :key="templateItem.Id"
                  :label="templateItem.Name" :value="templateItem.Name">
                </el-option>
              </el-select>
              <el-input v-else v-model="item.Name" placeholder="请输入配置项名称"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="" label-width="0px" :prop="'dbOptionsItems.' + index + '.Value'"
              :rules="rules.dbOptionsItemsValue">
              <el-input v-model="item.Value" placeholder="请输入配置项值"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="3">
            <el-button size="mini" @click.prevent="removeConfigFormItem(optionsType.DBOPTIONS, item)">删除
            </el-button>
          </el-col>
        </el-row>
        <el-form-item>
          <el-button @click="addConfigFormItem(optionsType.DBOPTIONS)">新增配置项</el-button>
        </el-form-item>
      </el-form>
    </div>

    <div class="title-table-options">
      <h3>TableOptions</h3>
      <el-divider></el-divider>
    </div>
    <div class="table-options">
      <el-form :ref="formRefs[2]" :model="tableConfigForm" style="margin-left:80px;" label-position="left"
        label-width="80px" size="mini" :rules="rules">
        <el-row :gutter="5" v-for="(item, index) in tableConfigForm.tableOptionsItems" :key="index">
          <el-col :span="11">
            <el-form-item :label="'配置项' + (index + 1)" :prop="'tableOptionsItems.' + index + '.Name'"
              :rules="rules.tableOptionsItemsName">
              <el-select v-if="isNameSelected" v-model="item.Name" filterable allow-create style="width:100%"
                placeholer="请选择配置项名称">
                <el-option v-for="templateItem in cachedDefaultConfigForm.tableOptionsItems" :key="templateItem.Id"
                  :label="templateItem.Name" :value="templateItem.Name">
                </el-option>
              </el-select>
              <el-input v-else v-model="item.Name" placeholder="请输入配置项名称"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="" label-width="0px" :prop="'tableOptionsItems.' + index + '.Value'"
              :rules="rules.tableOptionsItemsValue">
              <el-input v-model="item.Value" placeholder="请输入配置项值"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="3">
            <el-button size="mini" @click.prevent="removeConfigFormItem(optionsType.TABLEOPTIONS, item)">删除
            </el-button>
          </el-col>
        </el-row>
        <el-form-item>
          <el-button @click="addConfigFormItem(optionsType.TABLEOPTIONS)">新增配置项</el-button>
        </el-form-item>
      </el-form>
    </div>

    <div class="title-cf-options">
      <h3>CfOptions</h3>
      <el-divider></el-divider>
    </div>
    <div class="cf-options">
      <el-form :ref="formRefs[3]" :model="tableConfigForm" style="margin-left:80px;" label-position="left"
        label-width="80px" size="mini" :rules="rules">
        <el-row :gutter="5" v-for="(item, index) in tableConfigForm.cfOptionsItems" :key="index">
          <el-col :span="11">
            <el-form-item :label="'配置项' + (index + 1)" :prop="'cfOptionsItems.' + index + '.Name'"
              :rules="rules.cfOptionsItemsName">
              <el-select v-if="isNameSelected" v-model="item.Name" filterable allow-create style="width:100%"
                placeholer="请选择配置项名称">
                <el-option v-for="templateItem in cachedDefaultConfigForm.cfOptionsItems" :key="templateItem.Id"
                  :label="templateItem.Name" :value="templateItem.Name">
                </el-option>
              </el-select>
              <el-input v-else v-model="item.Name" placeholder="请输入配置项名称"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="" label-width="0px" :prop="'cfOptionsItems.' + index + '.Value'"
              :rules="rules.cfOptionsItemsValue">
              <el-input v-model="item.Value" placeholder="请输入配置项值"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="3">
            <el-button size="mini" @click.prevent="removeConfigFormItem(optionsType.CFOPTIONS, item)">删除
            </el-button>
          </el-col>
        </el-row>
        <el-form-item>
          <el-button @click="addConfigFormItem(optionsType.CFOPTIONS)">新增配置项</el-button>
        </el-form-item>
      </el-form>
    </div>

    <div class="save-table-config" style="text-align:center">
      <br />
      <el-button type="primary" @click="saveTableConfig()">保存</el-button>
      <el-button @click="discardChanges()">丢弃更改</el-button>
    </div>
  </div>
</template>

 
<script>
import {
  ListTableConfig,
  AddTableConfig,
  UpdateTableConfig,
  ListTableConfigWithDetailItems
} from '@/api/table_config'
import { waves } from '@/directive/waves'
import { validPartitionNumber } from '@/utils/validate'

export default {
  name: 'TableConfigTemplate',
  directives: { waves },
  props: {
    isNameSelected: Boolean,
    configName: {
      type: String,
      default: 'default'
    },
    asTemplate: {
      type: Boolean,
      default: true
    }
  },
  data() {
    return {
      CONFIG_TEMPLATE_NAME: 'default',
      optionsType: {
        DBOPTIONS: 1,
        TABLEOPTIONS: 2,
        CFOPTIONS: 3
      },
      genNextId: this.nextId(),
      tableConfigForm: this.newTableConfigForm(),
      cachedTableConfig: null,
      cachedDefaultConfigForm: this.newTableConfigForm(),
      formRefs: [
        'formBasicInfo',
        'formDbOptions',
        'formTableOptions',
        'formCfOptions'
      ],
      formCheckResults: [],
      queryParams: {
        Page: 1,
        Limit: 1,
        Name: 'default'
      },
      rules: {
        Name: [{ required: true, message: '配置名称不能为空' }],
        dbOptionsItemsName: [
          { required: true, message: '配置项名称不能为空', trigger: 'blur' },
          {
            validator: (rule, value, callback) => {
              return callback()
            },
            trigger: 'blur'
          }
        ],
        dbOptionsItemsValue: [
          { required: true, message: '配置项值不能为空', trigger: 'blur' }
        ],
        tableOptionsItemsName: [
          { required: true, message: '配置项名称不能为空', trigger: 'blur' }
        ],
        tableOptionsItemsValue: [
          { required: true, message: '配置项值不能为空', trigger: 'blur' }
        ],
        cfOptionsItemsName: [
          { required: true, message: '配置项名称不能为空', trigger: 'blur' }
        ],
        cfOptionsItemsValue: [
          { required: true, message: '配置项值不能为空', trigger: 'blur' }
        ]
      }
    }
  },
  created() {
    if (this.asTemplate == false) {
      this.queryParams.Name = 'default'
      this.fetchTableConfig(false, true)
    }
    this.queryParams.Name = this.configName
    this.fetchTableConfig(true)
  },
  watch: {
    configName(val, oldVal) {
      if (val == '') {
        this.tableConfigForm = this.newTableConfigForm()
        this.cachedTableConfig = null
        this.fillFormWithDefaultTableConfigOnCreate()
        return
      }
      this.queryParams.Name = val
      this.fetchTableConfig(true)
    }
  },
  methods: {
    fetchTableConfig(show_loading = true, fetch_template = false) {
      if (this.queryParams.Name == '') {
        return
      }
      var loading = null
      if (show_loading) {
        loading = this.fullScreenLoading('加载中...')
      }
      let params = JSON.parse(JSON.stringify(this.queryParams))
      ListTableConfigWithDetailItems(params)
        .then(response => {
          if (response.Data.Items.length >= 1) {
            if (fetch_template == false) {
              this.cachedTableConfig = response.Data.Items[0]
              this.tableConfigForm = this.fillFormWithTableConfig(
                this.cachedTableConfig
              )
            } else {
              this.cachedDefaultConfigForm = this.fillFormWithTableConfig(
                response.Data.Items[0]
              )
              this.fillFormWithDefaultTableConfigOnCreate()
            }
          }
        })
        .finally(() => {
          if (loading != null) {
            loading.close()
          }
        })
    },
    saveTableConfig() {
      this.formCheckResults = []
      this.formRefs.forEach(form => {
        this.checkForm(form)
      })

      Promise.all(this.formCheckResults)
        .then(() => {
          const loading = this.fullScreenLoading('正在保存..')
          let newTableConfig = this.fillTableConfigWithForm(
            this.tableConfigForm
          )
          let isConfigChanged = this.isTableConfigChanged(newTableConfig)
          if (isConfigChanged) {
            newTableConfig.Version += 1
          }
          let saveFunc
          if (newTableConfig.Id == undefined) {
            saveFunc = AddTableConfig
          } else {
            saveFunc = UpdateTableConfig
          }
          saveFunc(newTableConfig)
            .then(response => {
              this.$notify({
                title: 'Success',
                message: 'Save Successfully',
                type: 'success',
                duration: 2000
              })
              this.fetchTableConfig(false)
              if (this.asTemplate == false) {
                this.$parent.$parent.postSaved()
              }
            })
            .finally(() => {
              loading.close()
            })
        })
        .catch(e => {
          console.log(e)
        })
    },
    discardChanges() {
      if (this.cachedTableConfig == undefined) {
        this.tableConfigForm = this.newTableConfigForm()
      } else {
        this.tableConfigForm = this.fillFormWithTableConfig(
          this.cachedTableConfig
        )
      }
    },
    checkForm(formName) {
      let _self = this
      let result = new Promise(function(resolve, reject) {
        let form = _self.$refs[formName]
        if (form != undefined) {
          form.validate(valid => {
            if (valid) {
              resolve()
            } else {
              reject()
            }
          })
        } else {
          resolve()
        }
      })
      _self.formCheckResults.push(result)
    },
    fullScreenLoading(text) {
      const loading = this.$loading({
        lock: true,
        text: text,
        spinner: 'el-icon-loading',
        background: 'rgba(0, 0, 0, 0.7)'
      })
      return loading
    },
    getTableConfigItemsByType(configForm, type) {
      var items = null
      switch (type) {
        case this.optionsType.DBOPTIONS:
          items = configForm.dbOptionsItems
          break
        case this.optionsType.TABLEOPTIONS:
          items = configForm.tableOptionsItems
          break
        case this.optionsType.CFOPTIONS:
          items = configForm.cfOptionsItems
          break
        default:
          break
      }
      return items
    },
    addConfigFormItem(type) {
      let items = this.getTableConfigItemsByType(this.tableConfigForm, type)
      if (items == null) {
        return
      }
      items.push({
        Name: '',
        Value: '',
        Key: this.genNextId.next().value
      })
    },
    removeConfigFormItem(type, item) {
      let items = this.getTableConfigItemsByType(this.tableConfigForm, type)
      if (items == null) {
        return
      }
      let index = items.indexOf(item)
      if (index != -1) {
        items.splice(index, 1)
      }
    },
    newTableConfigForm() {
      return {
        Id: undefined,
        Name: this.configName,
        Version: 0,
        IsDefault: 0,
        dbOptionsItems: [],
        tableOptionsItems: [],
        cfOptionsItems: []
      }
    },
    fillFormWithDefaultTableConfigOnCreate() {
      if (this.configName === '') {
        this.tableConfigForm = JSON.parse(
          JSON.stringify(this.cachedDefaultConfigForm)
        )
        this.tableConfigForm.Id = undefined
        this.tableConfigForm.Name = ''
        this.tableConfigForm.Version = 0
        this.tableConfigForm.IsDefault = 0
      }
    },
    fillFormWithTableConfig(tableConfig) {
      let tempTableConfigForm = this.newTableConfigForm()
      if (tableConfig != null) {
        tempTableConfigForm.Id = tableConfig.Id
        tempTableConfigForm.Name = tableConfig.Name
        tempTableConfigForm.Version = tableConfig.Version
        tempTableConfigForm.IsDefault = tableConfig.IsDefault
        tempTableConfigForm.Desc = tableConfig.Desc
        let configItems = tableConfig.ConfigItems
        if (configItems == null) {
          return
        }
        for (let index = 0; index < configItems.length; index++) {
          let items = this.getTableConfigItemsByType(
            tempTableConfigForm,
            configItems[index].Type
          )
          items.push({
            Name: configItems[index].Name,
            Value: configItems[index].Value,
            Key: this.genNextId.next().value
          })
        }
      }
      return tempTableConfigForm
    },
    fillTableConfigWithForm(tableConfigForm) {
      let tempTableConfig = {}
      tempTableConfig.Id = tableConfigForm.Id
      tempTableConfig.Name = tableConfigForm.Name
      tempTableConfig.Version = tableConfigForm.Version
      if (this.asTemplate == true) {
        tempTableConfig.IsDefault = 1
      } else {
        tempTableConfig.IsDefault = 0
      }
      tempTableConfig.Desc = tableConfigForm.Desc
      tempTableConfig.ConfigItems = new Array()
      for (let key in this.optionsType) {
        let items = this.getTableConfigItemsByType(
          tableConfigForm,
          this.optionsType[key]
        )
        if (items != null) {
          for (let index = 0; index < items.length; index++) {
            tempTableConfig.ConfigItems.push({
              Name: items[index].Name,
              Value: items[index].Value,
              Type: this.optionsType[key]
            })
          }
        }
      }
      return tempTableConfig
    },
    isTableConfigChanged(newConfig) {
      if (!this.cachedTableConfig) {
        return true
      }
      let cachedTableConfig = JSON.parse(JSON.stringify(this.cachedTableConfig))
      let newTableConfig = JSON.parse(JSON.stringify(newConfig))
      if (
        cachedTableConfig.Name !== newConfig.Name ||
        cachedTableConfig.ConfigItems.length !==
          newTableConfig.ConfigItems.length
      ) {
        return true
      }

      let compareFunc = function(item1, item2) {
        if (item1.Name < item2.Name) {
          return -1
        }
        if (item1.Name > item2.Name) {
          return 1
        }
        return 0
      }
      cachedTableConfig.ConfigItems.sort(compareFunc)
      newTableConfig.ConfigItems.sort(compareFunc)
      for (let i = 0; i < cachedTableConfig.ConfigItems.length; ++i) {
        let configItem = cachedTableConfig.ConfigItems[i]
        if (
          configItem.Name === newTableConfig.ConfigItems[i].Name &&
          configItem.Value === newTableConfig.ConfigItems[i].Value &&
          configItem.Type === newTableConfig.ConfigItems[i].Type
        ) {
          continue
        }
        return true
      }
      return false
    },
    *nextId() {
      let current_id = 0
      while (true) {
        current_id++
        yield current_id
      }
    }
  }
}
</script>
