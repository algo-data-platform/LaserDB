<template>
  <el-row :gutter="20">
    <el-col :span="12">
      <div class="board-column">
        <div class="board-column-header" style="background: #f9944a">
          候选特征
        </div>
        <draggable
          :list="unselected"
          :group="{ name: 'row' }"
          :key="1"
          class="board-column-content"
        >
          <div v-for="element in unselected" :key="element.Name" class="board-item">
            {{ element.Name }} {{ element.Id }}
          </div>
        </draggable>
      </div>
    </el-col>
    <el-col :span="12">
      <div class="board-column">
        <div class="board-column-header" style="background: #2ac06d">
          已选特征
        </div>
        <draggable
          :list="selected"
          :group="{ name: 'row' }"
          :key="2"
          class="board-column-content"
          @change="log"
        >
          <div v-for="element in selected" :key="element.Name" class="board-item">
            {{ element.Name }} {{ element.Id }}
          </div>
        </draggable>
      </div>
    </el-col>
  </el-row>
</template>

<script>

import draggable from 'vuedraggable'
import { getList as getFeatureList } from '@/api/feature'

export default {
  components: {
    draggable
  },
  props: {
    index: {
      type: Number
    },
    did: {
      type: Number,
      default() {
        return undefined
      }
    },
    initSelected: {
      type: Array,
      default() {
        return []
      }
    }
  },
  data() {
    return {
      selected: [],
      unselected: []
    }
  },
  created() {
    this.fetchData()
  },
  mounted() {
    this.log(null)
  },
  methods: {
    fetchData() {
      getFeatureList({ Status: 1, Did: this.did }).then(response => {
        const features = response.Data.Items
        features.forEach(feature => {
          if (this.initSelected.indexOf(feature.Id) !== -1) {
            this.selected.push(feature)
          } else {
            this.unselected.push(feature)
          }
        })
      })
    },
    log: function(evt) {
      this.$emit('updateData', this.index, this.unselected, this.selected)
    }
  }
}
</script>
<style lang="scss" scoped>
.board-column {
  min-width: 300px;
  min-height: 100px;
  height: auto;
  overflow: hidden;
  background: #f0f0f0;
  border-radius: 3px;

  .board-column-header {
    height: 50px;
    line-height: 50px;
    overflow: hidden;
    padding: 0 20px;
    text-align: center;
    color: #fff;
    border-radius: 3px 3px 0 0;
  }

  .board-column-content {
    height: auto;
    overflow: hidden;
    border: 10px solid transparent;
    min-height: 60px;
    display: flex;
    justify-content: flex-start;
    flex-direction: column;
    align-items: center;

    .board-item {
      cursor: pointer;
      width: 100%;
      height: 64px;
      margin: 5px 0;
      background-color: #fff;
      text-align: left;
      line-height: 54px;
      padding: 5px 10px;
      box-sizing: border-box;
      box-shadow: 0px 1px 3px 0 rgba(0, 0, 0, 0.2);
    }
  }
}
</style>
