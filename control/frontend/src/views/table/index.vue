<template>
  <div class="app-container">
    <div class="filter-container">
      <el-form ref="searchForm" :model="listQuery" label-position="left" style="margin-left:20px;" >
        <el-row :gutter="15">
          <el-col :span="9">
            <el-form-item label="数据库" label-width="60px" prop="SearchDataBase">
              <el-input v-model="listQuery.DatabaseName" clearable/>
            </el-form-item>
          </el-col>
          <el-col :span="9">
            <el-form-item label="数据表" label-width="60px" prop="SearchTable">
              <el-input v-model="listQuery.TableName" clearable/>
            </el-form-item>
          </el-col>
          <el-col :span="3">
            <el-button class="filter-item" style="width: 90px; margin-left: 10px; " type="primary" icon="el-icon-search" @click="searchTable()">
              搜索
            </el-button>
          </el-col>
        </el-row>
      </el-form>
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-edit"
        @click="handleCreate">
        添加
      </el-button>
      <el-button class="filter-item" style="margin-left: 10px;" type="primary" icon="el-icon-upload"
        :loading="consulSynchronizing" @click="synchronizeDataToConsul">
        同步
      </el-button>
      <el-checkbox v-model="showDesc" class="filter-item" style="margin-left:15px;" @change="tableKey=tableKey+1">
        描述
      </el-checkbox>
    </div>
    <el-table
      :key="tableKey"
      v-loading="listLoading"
      :data="list"
      element-loading-text="Loading"
      border
      fit
      highlight-current-row>
      <el-table-column align="center" label="ID" width="95" type="index"></el-table-column>
      <el-table-column label="Laser 数据库">
        <template slot-scope="scope">
          <span>{{ scope.row.DatabaseName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Laser 数据表">
        <template slot-scope="scope">
          <span>{{ scope.row.Name }}</span>
        </template>
      </el-table-column>
      <el-table-column label="数据中心">
        <template slot-scope="scope">
          <span>{{ scope.row.DcName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="目标数据中心">
        <template slot-scope="scope">
          <span>{{ scope.row.DistDcName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="分区数">
        <template slot-scope="scope">
          <span>{{ scope.row.PartitionNumber }}</span>
        </template>
      </el-table-column>
      <el-table-column label="Ttl">
        <template slot-scope="scope">
          <span>{{ scope.row.Ttl }}</span>
        </template>
      </el-table-column>
      <el-table-column label="配置">
        <template slot-scope="scope">
          <span>{{ scope.row.ConfigName }}</span>
        </template>
      </el-table-column>
      <el-table-column label="状态">
        <template slot-scope="scope">
          <span>{{ statusText[scope.row.Status] }}</span>
        </template>
      </el-table-column>
      <el-table-column label="禁止所有命令">
        <template slot-scope="scope">
          <span>{{ offOnText[scope.row.DenyAll] }}</span>
        </template>
      </el-table-column>
      <el-table-column label="描述" v-if="showDesc">
        <template slot-scope="scope">
          <span>{{ scope.row.Desc }}</span>
        </template>
      </el-table-column>
      <el-table-column label="操作" align="center" width="150" class-name="small-padding fixed-width">
        <template slot-scope="{row}">
          <el-button type="primary" size="mini" @click="handleUpdate(row)">修改</el-button>
          <el-button type="primary" size="mini" @click="deleteTable(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <div class="pagination-class">
      <el-pagination
        :background="true"
        @current-change="handleCurrentChange"
        :current-page="listQuery.Page"
        :page-size="listQuery.Limit"
        layout="total, prev, pager, next, jumper"
        :total="listTotal"
      ></el-pagination>
    </div>

    <el-dialog :title="textMap[dialogStatus]" :visible.sync="dialogFormVisible" :close-on-click-modal="false">
      <h3>基本信息</h3>
      <el-divider></el-divider>
      <el-form
        :ref="formRefs[0]"
        :rules="rules"
        :model="temp"
        label-position="left"
        label-width="140px"
        style="width: 400px; margin-left:50px;">
        <el-form-item label="Laser 数据库" prop="DatabaseName">
          <el-select
            class="el-select"
            v-model="temp.DatabaseName"
            :loading="listDBLoading"
            placeholder="请选择"
            @visible-change="fetchDatabase"
            @change="getDatabaseIdByName(temp.DatabaseName)"
            v-bind:disabled="dialogStatus=='update'">
            <el-option
              v-for="item in databaseList"
              :key="item.Id"
              :label="item.Name"
              :value="item.Name"></el-option>
          </el-select>
        </el-form-item>
        <el-form-item label="Laser 数据表" prop="Name">
          <el-input v-model="temp.Name" v-bind:disabled="dialogStatus=='update'" />
        </el-form-item>
        <el-form-item label="分区数" prop="PartitionNumber">
          <el-input
            v-model.number="temp.PartitionNumber"
            type="number"
            min="1"
            oninput="value=value.replace(/[^\d]/g,'')"
            v-bind:disabled="dialogStatus=='update'"
          />
        </el-form-item>
        <el-form-item label="Ttl" prop="Ttl">
          <el-input
            v-model.number="temp.Ttl"
            type="number"
            oninput="value=value.replace(/[^\d]/g,'')"
          />
        </el-form-item>
        <el-form-item label="数据中心" prop="DcName">
          <el-select
            class="el-select"
            v-model="temp.DcName"
            :loading="listDcLoading"
            placeholder="请选择"
            @visible-change="fetchDcData"
            @change="getDcIdByName(temp.DcName, 'dc')">
            <el-option v-for="item in dcList" :key="item.Id" :label="item.Name" :value="item.Name"></el-option>
          </el-select>
        </el-form-item>
        <el-form-item label="目标数据中心" prop="DistDcName">
          <el-select
            class="el-select"
            v-model="temp.DistDcName"
            :loading="listDcLoading"
            placeholder="请选择"
            @visible-change="fetchDcData"
            @change="getDcIdByName(temp.DistDcName, 'distDc')">
            <el-option v-for="item in dcList" :key="item.Id" :label="item.Name" :value="item.Name"></el-option>
          </el-select>
        </el-form-item>

        <el-form-item label="配置" prop="ConfigName">
          <el-select
            class="el-select"
            v-model="temp.ConfigName"
            :loading="listConfigLoading"
            placeholder="请选择"
            @visible-change="fetchConfigData"
            @change="getConfigIdByName(temp.ConfigName)"
          >
            <el-option
              v-for="item in configList"
              :key="item.Id"
              :label="item.Name"
              :value="item.Name"
            ></el-option>
          </el-select>
        </el-form-item>
        <el-form-item label="状态" prop="Status">
          <el-switch
            v-model="temp.Status"
            :active-text="statusText[1]"
            :inactive-text="statusText[0]"
            :active-value="1"
            :inactive-value="0"
            active-color="#13ce66"
            inactive-color="#ff4949"
          ></el-switch>
        </el-form-item>
        <el-form-item label="禁止所有命令" prop="DenyAll">
          <el-switch
            v-model="temp.DenyAll"
            :active-text="offOnText[1]"
            :inactive-text="offOnText[0]"
            :active-value="1"
            :inactive-value="0"
            active-color="#13ce66"
            inactive-color="#ff4949"
          ></el-switch>
        </el-form-item>
        <el-form-item label="绑定边缘节点" prop="selectedEdgeNodeOptions">
          <el-cascader
            class="el-select"
            v-model="selectedEdgeNodeOptions"
            :options="edgeNodeOptions"
            :props="cascaderProps"
            :loading="listEdgeNodesLoading"
            @visible-change="handleEdgeNodeCascaderVisibleChanged"
            @change="handleEdgeNodeCascaderChanged"
          ></el-cascader>
        </el-form-item>
        <el-form-item label="边缘节点流量比例" prop="EdgeFlowRatio">
          <el-input
            v-model.number="temp.EdgeFlowRatio"
            type="number"
            min="0"
            max="100"
            oninput="value=value.replace(/[^\d]/g,'')"
            placeholder="比例(0-100)"
            v-bind:disabled="selectedEdgeNodeOptions.length == 0"
          ></el-input>
        </el-form-item>
        <el-form-item label="描述" prop="Desc">
          <el-input v-model="temp.Desc" type="textarea" />
        </el-form-item>
      </el-form>

      <h3>限流设置</h3>
      <el-divider></el-divider>
      <el-form
        :ref="formRefs[1]"
        :model="temp"
        style="margin-left:50px;"
        label-position="left"
        label-width="80px"
        size="mini"
        :rules="rules"
      >
        <el-row :gutter="5" v-for="(item, index) in temp.TrafficLimits" :key="index">
          <el-col :span="8">
            <el-form-item
              :label="'命令' + (index + 1)"
              :prop="'TrafficLimits.' + index + '.Name'"
              :rules="rules.CommandLimitName"
            >
              <el-select
                v-model="item.Name"
                style="width:100%"
                placeholder="命令"
                @change="setCommandType(item)"
              >
                <el-option
                  v-for="command in commandList"
                  :key="command.Name"
                  :label="command.Name"
                  :value="command.Name"
                ></el-option>
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="5">
            <el-form-item
              label
              label-width="0px"
              :prop="'TrafficLimits.' + index + '.LimitType'"
              :rules="rules.CommandLimitName"
            >
              <el-select
                v-model="item.LimitType"
                placeholder="限制类型"
                v-bind:disabled="item.OperationType==SINGLE_OPERATION"
              >
                <el-option
                  v-for="type in limitTypeList"
                  :key="type.Vaule"
                  :label="type.Name"
                  :value="type.Value"
                ></el-option>
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="6">
            <el-form-item
              label
              label-width="0px"
              :prop="'TrafficLimits.' + index + '.LimitValue'"
              :rules="rules.CommandLimitValue"
            >
              <el-input
                v-model.number="item.LimitValue"
                type="number"
                min="0"
                max="100"
                oninput="value=value.replace(/[^\d]/g,'')"
                placeholder="通过率(0-100)"
              ></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="3">
            <el-button size="mini" @click.prevent="removeCommandLimit(item)">删除</el-button>
          </el-col>
        </el-row>
        <el-form-item>
          <el-button @click="addCommandLimit()">新增设置</el-button>
        </el-form-item>
      </el-form>

      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogFormVisible = false">取消</el-button>
        <el-button type="primary" @click="dialogStatus==='create'? createData() : updateData()">提交</el-button>
      </div>
    </el-dialog>
  </div>
</template>

<style>
.el-select {
  width: 100%;
}
</style>

<script>
import {
  ListTable,
  ListTableDetail,
  AddTable,
  UpdateTable,
  DeleteTable,
  SynchronizeDataToConsul,
  ListCommands,
} from "@/api/table";
import { ListDatabase } from "@/api/database";
import { ListTableConfig } from "@/api/table_config";
import { ListDc } from "@/api/dc";
import { waves } from "@/directive/waves";
import {
  validPartitionNumber,
  validCommandLimitValue,
  validEdgeFlowRatio,
} from "@/utils/validate";
import { getList } from "@/api/node";

export default {
  directives: { waves },
  data() {
    return {
      cascaderProps: {
        multiple: true,
      },
      SINGLE_OPERATION: 1,
      MULTIPLE_OPERATION: 2,
      LIMIT_KPS: 0,
      LIMIT_QPS: 1,
      list: null,
      listTotal: undefined,
      tableDelete: {
        TableId: undefined,
      },
      currentPage: 1,
      listLoading: true,
      tableKey: 0,
      showDesc: true,
      listQuery: {
        Page: 1,
        Limit: 20,
        DatabaseName: "",
        TableName: ""
      },
      listQueryDetail: {
        DatabaseId: undefined,
        TableId: undefined,
      },
      dialogFormVisible: false,
      textMap: {
        create: "创建数据表",
        update: "修改数据表",
      },
      dialogStatus: "create",
      statusText: {
        0: "禁用",
        1: "启用",
      },
      offOnText: {
        0: "关闭",
        1: "开启",
      },
      listDBLoading: true,
      databaseList: null,
      listConfigLoading: true,
      listDcLoading: true,
      configList: null,
      dcList: null,
      consulSynchronizing: false,
      commandList: [],
      limitTypeList: [
        {
          Name: "KPS",
          Value: 0,
        },
        {
          Name: "QPS",
          Value: 1,
        },
      ],
      listNodeParams: {
        Page: undefined,
        Limit: undefined,
        GroupId: undefined,
        IsEdgeNode: true,
      },
      listEdgeNodesLoading: true,
      edgeNodeOptions: undefined,
      selectedEdgeNodeOptions: [],
      temp: this.newTableInfo(),
      formRefs: ["formBasicInfo", "formCommandSet"],
      formCheckResults: [],
      rules: {
        DatabaseName: [{ required: true, message: "数据库名称是必填项" }],
        ConfigName: [{ required: true, message: "配置名称是必填项" }],
        Name: [{ required: true, message: "表名是必填项" }],
        DcName: [{ required: true, min: 1, message: "数据中心是必填项目" }],
        DistDcName: [{ required: true, min: 1, message: "目标数据中心是必填项目" }],
        PartitionNumber: [
          { required: true, message: "分区数是必填项" },
          { type: "number", message: "请输入数字格式" },
          { validator: validPartitionNumber, trigger: "blur" },
        ],
        CommandLimitName: [
          { required: true, message: "必选项不能为空", trigger: "blur" },
        ],
        CommandLimitValue: [
          { required: true, message: "必填项不能为空", trigger: "blur" },
          { type: "number", message: "请输入数字格式" },
          { validator: validCommandLimitValue, trigger: "blur" },
        ],
        EdgeFlowRatio: [
          { required: true, message: "必填项不能为空", trigger: "blur" },
          { type: "number", message: "请输入数字格式" },
          { validator: validEdgeFlowRatio, trigger: "blur" },
        ],
      },
    };
  },
  created() {
    this.fetchData();
    this.resetTemp();
    this.fetchCommands();
    this.fetchEdgeNodesOptions();
  },
  methods: {
    fetchData() {
      this.listLoading = true;
      ListTable(this.listQuery).then((response) => {
        this.list = response.Data.Items;
        this.listTotal = response.Data.Total;
        this.listLoading = false;
      });
    },
    fetchDataDetail(row) {
      this.listQueryDetail.DatabaseId = row.DatabaseId;
      this.listQueryDetail.TableId = row.Id;
      ListTableDetail(this.listQueryDetail).then((response) => {
        this.temp = response.Data.Items[0];
        this.selectedEdgeNodeOptions = this.edgeNodeInfosToOptions(
          this.temp.BindEdgeNodes
        );
      });
    },
    fetchCommands() {
      ListCommands().then((response) => {
        this.commandList = response.Data.Items;
      });
    },
    resetTemp() {
      this.temp = this.newTableInfo();
    },
    handleCreate() {
      this.resetTemp();
      this.selectedEdgeNodeOptions = [];
      this.dialogFormVisible = true;
      this.dialogStatus = "create";
      this.$nextTick(() => {
        this.formRefs.forEach((form) => {
          this.clearFormValidate(form);
        });
      });
    },
    createData() {
      this.formCheckResults = [];
      this.formRefs.forEach((form) => {
        this.checkForm(form);
      });

      Promise.all(this.formCheckResults)
        .then(() => {
          this.temp.BindEdgeNodeIds = this.edgeNodeOptionsToNodeIds(
            this.selectedEdgeNodeOptions
          );
          this.temp.BindEdgeNodes = null;
          AddTable(this.temp).then((response) => {
            this.$notify({
              title: "Success",
              message: "Created Successfully",
              type: "success",
              duration: 2000,
            });
            this.fetchData();
          });
          this.dialogFormVisible = false;
        })
        .catch((e) => {
          console.log(e);
        });
    },
    handleUpdate(row) {
      this.dialogFormVisible = true;
      this.dialogStatus = "update";
      this.$nextTick(() => {
        this.formRefs.forEach((form) => {
          this.clearFormValidate(form);
        });
      });
      this.fetchDataDetail(row);
    },
    updateData() {
      console.log(this.temp);
      this.formCheckResults = [];
      this.formRefs.forEach((form) => {
        this.checkForm(form);
      });

      Promise.all(this.formCheckResults)
        .then(() => {
          this.temp.BindEdgeNodeIds = this.edgeNodeOptionsToNodeIds(
            this.selectedEdgeNodeOptions
          );
          this.temp.BindEdgeNodes = null;
          UpdateTable(this.temp).then((response) => {
            this.$notify({
              title: "Success",
              message: "Update Successfully",
              type: "success",
              duration: 2000,
            });
            this.fetchData();
          });
          this.dialogFormVisible = false;
        })
        .catch((e) => {
          console.log(e);
        });
    },
    deleteTable(row) {
      this.tableDelete.TableId = row.Id;
      this.$confirm("确定删除?", "提示", {
        confirmButtonText: "确定",
        cancelButtonText: "取消",
        type: "warning",
      }).then(() => {
        DeleteTable(this.tableDelete).then((response) => {
          this.$notify({
            title: "Success",
            message: "Delete Successfully",
            type: "success",
            duration: 2000,
          });
          this.fetchData();
        });
      });
    },
    searchTable() {
      this.listQuery.Page = 1
      this.fetchData();
    },
    synchronizeDataToConsul() {
      this.$confirm("确认要同步数据表及配置信息？", "提示", {
        confirmButtonText: "确定",
        cancelButtonText: "取消",
        type: "warning",
      })
        .then(() => {
          this.consulSynchronizing = true;
          SynchronizeDataToConsul()
            .then((reponse) => {
              this.$notify({
                title: "Success",
                message: "Synchronize Successfully",
                type: "success",
                duration: 2000,
              });
            })
            .finally((e) => {
              this.consulSynchronizing = false;
            });
        })
        .catch(() => {});
    },
    fetchDatabase(visibleFlag) {
      if (visibleFlag) {
        this.listDBLoading = true;
        ListDatabase().then((response) => {
          this.databaseList = response.Data.Items;
          this.listDBLoading = false;
        });
      }
    },
    getDatabaseIdByName(name) {
      for (let index = 0; index < this.databaseList.length; ++index) {
        if (name == this.databaseList[index].Name) {
          this.temp.DatabaseId = this.databaseList[index].Id;
          break;
        }
      }
    },
    fetchDcData(visibleFlag) {
      if (visibleFlag) {
        this.listDcLoading = true;
        ListDc()
          .then((response) => {
            this.dcList = response.Data.Items;
          })
          .finally(() => {
            this.listDcLoading = false;
          });
      }
    },
    fetchConfigData(visibleFlag) {
      if (visibleFlag) {
        this.listConfigLoading = true;
        ListTableConfig()
          .then((response) => {
            this.configList = response.Data.Items;
          })
          .finally(() => {
            this.listConfigLoading = false;
          });
      }
    },
    getConfigIdByName(name) {
      for (let index = 0; index < this.configList.length; ++index) {
        if (name == this.configList[index].Name) {
          this.temp.ConfigId = this.configList[index].Id;
          break;
        }
      }
    },
    getDcIdByName(name, type) {
      for (let index = 0; index < this.dcList.length; ++index) {
        if (name == this.dcList[index].Name) {
          if (type == 'dc') {
            this.temp.DcId = this.dcList[index].Id;
          } else {
            this.temp.DistDcId = this.dcList[index].Id;
          }
          break;
        }
      }
    },
    fetchEdgeNodesOptions() {
      getList().then((response) => {
        this.edgeNodeOptions = new Array();
        let groups = response.Data;
        groups.forEach((group) => {
          let groupOption = {
            value: group.Id,
            label: group.GroupName,
            children: [],
          };
          group.Nodes.forEach((node) => {
            if (node.IsEdgeNode) {
              let nodeOption = {
                value: node.Id,
                label:
                  "node" +
                  node.NodeId +
                  "(" +
                  node.Host +
                  ":" +
                  node.Port +
                  ")",
              };
              groupOption.children.push(nodeOption);
            }
          });
          if (groupOption.children.length > 0) {
            this.edgeNodeOptions.push(groupOption);
          }
        });
      });
    },
    handleEdgeNodeCascaderVisibleChanged(visibleFlag) {
      if (visibleFlag) {
        this.fetchEdgeNodesOptions();
      }
    },
    handleEdgeNodeCascaderChanged(value) {
      if (value.length == 0) {
        this.temp.EdgeFlowRatio = 0;
      }
    },
    handleCurrentChange(val) {
      this.listQuery.Page = val;
      this.fetchData();
    },
    addCommandLimit() {
      let limits = this.temp.TrafficLimits;
      if (limits == null) {
        return;
      }
      limits.push({
        Name: "",
        LimitType: "",
        LimitValue: undefined,
      });
    },
    removeCommandLimit(item) {
      let limits = this.temp.TrafficLimits;
      if (limits == null) {
        return;
      }
      let index = limits.indexOf(item);
      if (index != -1) {
        limits.splice(index, 1);
      }
    },
    setCommandType(item) {
      for (let index = 0; index < this.commandList.length; ++index) {
        if (item.Name == this.commandList[index].Name) {
          item.OperationType = this.commandList[index].OperationType;
          if (item.OperationType == this.SINGLE_OPERATION) {
            item.LimitType = this.LIMIT_QPS;
          }
          break;
        }
      }
    },
    checkForm(formName) {
      let _self = this;
      let result = new Promise(function (resolve, reject) {
        let form = _self.$refs[formName];
        if (form != undefined) {
          form.validate((valid) => {
            if (valid) {
              resolve();
            } else {
              reject();
            }
          });
        } else {
          resolve();
        }
      });
      _self.formCheckResults.push(result);
    },
    clearFormValidate(formName) {
      let form = this.$refs[formName].clearValidate();
    },
    edgeNodeInfosToOptions(nodeInfos) {
      let selectedEdgeNodeOptions = new Array();
      nodeInfos.forEach((node) => {
        let group_node_id = new Array();
        group_node_id[0] = node.GroupId;
        group_node_id[1] = node.Id;
        selectedEdgeNodeOptions.push(group_node_id);
      });
      return selectedEdgeNodeOptions;
    },
    edgeNodeOptionsToNodeIds(options) {
      let nodeIds = new Array();
      options.forEach((option) => {
        nodeIds.push(option[1]);
      });
      return nodeIds;
    },
    newTableInfo() {
      return {
        Id: undefined,
        Name: "",
        Status: 0,
        DenyAll: 0,
        PartitionNumber: undefined,
        Ttl: 0,
        Desc: "",
        DcId: undefined,
        DcName: "",
        DistDcId: undefined,
        DistDcName: "",
        DatabaseId: undefined,
        ConfigId: undefined,
        DatabaseName: "",
        ConfigName: "",
        TrafficLimits: [],
        BindEdgeNodes: [],
        BindEdgeNodeIds: [],
        EdgeFlowRatio: 0,
      };
    },
  },
};
</script>
