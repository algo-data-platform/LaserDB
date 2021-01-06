<template>
  <div class="app-container">
    <el-tabs v-model="activeName">
      <el-form ref="searchForm" :model="filterParams" style="margin-left:20px;" >
        <el-row :gutter="0">
          <el-col :span="2">
            <el-checkbox class="filter-item" style="margin-top:10px;margin-left:0px;"
              @change="checked => {filterParams.enable = checked}">
              过滤数据表:
            </el-checkbox>
          </el-col>
          <el-col :span="9">
            <el-form-item>
              <el-input v-model="filterParams.tableName" clearable/>
            </el-form-item>
          </el-col>
        </el-row>
      </el-form>
      <el-tab-pane :label="group.DescName" :name="group.GroupName" v-for="group in list">
        <nodeView :nodeList="group.Nodes" :shardList="shardData" :shards="shards" :relations="relations"
          :groupName="group.GroupName" :groupId="group.Id"></nodeView>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script>
import { getList, getShardList } from '@/api/node'
import { waves } from '@/directive/waves'
import nodeView from './node_view'

export default {
  components: {
    nodeView
  },
  directives: { waves },
  data() {
    return {
      activeName: 'empty',
      list: [],
      shardData: {},
      shards: {},
      relations: {},
      nodeTimeoutId: undefined,
      shardTimeoutId: undefined,
      filterParams: {
        tableName: "",
        enable: false
      }
    }
  },
  created() {
    this.fetchNodeList()
    this.fetchShardList()
  },
  beforeDestroy() {
    clearTimeout(this.nodeTimeoutId)
    clearTimeout(this.shardTimeoutId)
  },
  methods: {
    fetchNodeListOnce() {
      getList().then(response => {
        this.list = response.Data

        if (this.filterParams.enable) {
          this.list.forEach(group => {
            group.Nodes = group.Nodes.filter(node => {
              if (node.Desc.includes(this.filterParams.tableName)) {
                return node
              }
            })
          })
        }
        
        this.list.forEach(group => {
          group.Nodes = group.Nodes.sort(function(node1, node2) {
            return node1.NodeId - node2.NodeId
          })
        })
        if (this.list.length > 0 && this.activeName === 'empty') {
          this.activeName = this.list[0].GroupName
        }
      })
    },
    fetchNodeList() {
      this.fetchNodeListOnce()
      this.nodeTimeoutId = setTimeout(this.fetchNodeList, 3000)
    },
    fetchShardList() {
      getShardList().then(response => {
        this.shardData = response.Data.groupShards
        this.shards = response.Data.shards
        this.relations = response.Data.relations
      })
      this.shardTimeoutId = setTimeout(this.fetchShardList, 15000)
    }
  }
}
</script>
