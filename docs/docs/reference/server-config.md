---
id: server-config
title: Laser Server 配置 
sidebar_label: Laser Server 配置
slug: /reference/server-config
---

对于 Laser Server 的配置主要是由两部分组成，分别是[服务节点配置](/) 和服务进程配置，对于服务节点配置是通过 Laser Control 配置，
用户可以根据部署集群的机器性能灵活的配置每个节点。服务进程配置一般都是与控制相关的配置，与机器资源无关的配置，通过在进程启动的时候指定，
一般都是通过 gflags 方式指定。

服务节点配置
----------

服务节点配置作用域分别是全局、集群组、服务节点三个级别，对于某个服务节点的配置最终生效的配置优先级 服务节点 > 集群组 > 全局，默认情况下 Laser Control
中只有一个全局配置，用户可以根据部署的机器资源来创建集群组、服务节点级的配置进行优化整体的集群性能，以下是目前支持的服务节点配置：

| 配置名称 | 默认值 | 说明 |
| :-- | :-- | :-- |
| BlockCacheSizeGb |  2  | 服务节点最大允许的内存缓存大小, 建议设置为系统内存 2/3 |
| HighPriPoolRatio |  0.5  | 缓存池中高优先的 cache 比例 |
| StrictCapacityLimit | true  | 是否强制限制内存大小 |
| NumShardBits | 6  | Cache 分片使用的前缀位数，代表分片的个数 |
| WriteBufferSizeGb | 1  | 写 Buffer 最大大小 |

一般情况下只需要根据系统内存的大小调整 `BlockCacheSizeGb` 和 `WriteBufferSizeGb` 大小即可，其他参数可以不做调整

服务进程配置
----------
