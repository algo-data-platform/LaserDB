<template>
  <div>
    <el-popover
      placement="right"
      width="560"
      trigger="click"
      @show="showDetail">
      <div v-if="displayDetail">
        <el-divider style="margin: 0px">当前节点分片信息</el-divider>
        <el-row>
          <el-col :span="4" >集群分组:</el-col>
          <el-col :span="8">{{shardInfo.groupName}}</el-col>
          <el-col :span="4" >节点 ID:</el-col>
          <el-col :span="8">{{shardInfo.nodeId}}</el-col>
        </el-row>
        <el-row>
          <el-col :span="4" >当前版本:</el-col>
          <el-col :span="8">{{shardInfo.baseVersionHash}}</el-col>
          <el-col :span="4" >更新序列:</el-col>
          <el-col :span="8">{{shardInfo.seqNo}}</el-col>
        </el-row>
        <el-divider style="margin: 0px">其他备份节点信息</el-divider>
        <el-row :key="relation.shardHash" v-for="relation in shardInfo.relations">
          <el-col :span="3">
            <div  :style="relation | shardStyle" class="shard" >
              <span style="font-weight: 700; margin: 2px">{{relation.shardId}}</span>
              <div class="shard-service" v-if="relation.serviceState !== 1"><i style="font-weight: bold" class="el-icon-close"></i></div>
              <div class="shard-data" v-if="!relation.noDiff && relation.role === 2"><i style="font-weight: bold" class="el-icon-loading"></i></div>
            </div>
          </el-col>
          <el-col :span="21">
            <el-row>
              <el-col :span="4" >集群分组:</el-col>
              <el-col :span="8">{{relation.groupName}}</el-col>
              <el-col :span="4" >节点 ID:</el-col>
              <el-col :span="8">{{relation.nodeId}}</el-col>
            </el-row>
            <el-row>
              <el-col :span="4" >当前版本:</el-col>
              <el-col :span="8">{{relation.baseVersionHash}}</el-col>
              <el-col :span="4" >更新序列:</el-col>
              <el-col :span="8">{{relation.seqNo}}</el-col>
            </el-row>
          </el-col>
        </el-row>
      </div>
      <div :style="shardInfo|shardStyle" slot="reference" class="shard">
        <span style="font-weight: 700; margin: 2px">{{shardInfo.shardId}}</span>
        <div class="shard-service" v-if="shardInfo.serviceState !== 1"><i style="font-weight: bold" class="el-icon-close"></i></div>
        <div class="shard-data" v-if="!shardInfo.noDiff && shardInfo.role === 2"><i style="font-weight: bold" class="el-icon-loading"></i></div>
      </div>
    </el-popover>
  </div>
</template>

<style>
  .shard {
    height: 50px;
    width: 50px;
    margin: 2px;
    position: relative;
  }
  .shard-service {
    position: absolute;
    right: 0px;
    bottom: 0px;
  }
  .shard-data {
    position: absolute;
    right: 20px;
    bottom: 0px;
  }
</style>

<script>
import { waves } from '@/directive/waves'

export default {
  directives: { waves },
  props: {
    shardInfo: {
      type: Object,
      default() {
        return {}
      }
    }
  },
  filters: {
    shardStyle: function(shard) {
      return shard.role === 1 ? 'background:#80c27e' : 'background:#5592c2'
    }
  },
  data() {
    return {
      displayDetail: false
    }
  },
  created() {
  },
  methods: {
    showDetail() {
      this.displayDetail = true
    }
  }
}
</script>
