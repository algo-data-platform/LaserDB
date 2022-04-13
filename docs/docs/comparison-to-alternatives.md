---
id: comparison-to-alternatives
title: 常见数据库对比
sidebar_label: 常见数据库对比
slug: /comparison-to-alternatives
---

## LaserDB vs. Redis

### 应用场景

Redis 应用在高性能低延迟的计算服务中，一般用作数据缓存或者借助于 redis 支持的复制数据类型做一些逻辑运算, 由于是一种内存数据结构数据库，
所以很难存储大容量的数据.

LaserDB 在数据模型上与 Redis 类似支持多种数据类型，并且性能上大部分场景与 Redis 接近，但是 LaserDB 是基于 RocksDB 存储引擎开发的分布式数据库，
可以实现大容量存储，适用于数据规模大的应用场景，比如算法工程中用来存储特征数据、模型数据，并且对于算法工程中或者其他应用工程应用中都有例行化批量更新数据库的需求，
LaserDB 通过原生支持批量导入可以快速无缝的更新数据库， 这个功能特征对于有定时全量更新 Cache 或者定时更新算法模型、特征的需求非常有帮助.

### 数据模型

Redis 支持 String, Counter, List, Set, Zset, Map 等多种数据类型，并且都是基于内存来实现，对应的水平扩展是采用 Slot 的方式扩展，对应的 Redis 集群解决方案
目前是采用 Redis Cluster 解决

LaserDB 在逻辑角度有数据库、数据表的概念来满足多租户的需求，可以根据不同的业务场景对不同的数据表进行灵活的优化，并且为了满足业务处理的方便,
LaserDB 具体的 KV 结构也支持 String, Counter, List, Set, Zset, Map 常用的类型, 并且 LaserDB 通过 Proxy 服务可以方便的兼容 Redis client
，可以方便的将使用 Redis 的业务无缝切换到 LaserDB 中。

### 总结

整体上 LaserDB 可以满足 Redis 高性能低延迟的基础上，弥补 Redis 在存储大容量数据上或者批量获取、批量更新上的不足。LaserDB 在架构设计上就很好的支持了集群水平扩展、
动态扩容缩容、自动容灾，单个 LaserDB 集群可以很好的满足公司各种应用场景。

下列场景使用 LaserDB 更好：

- 存储数据量大，内存容量无法满足
- 数据库中的数据有批量更新的需求
- 在线服务对获取数据有批量获取（mget）的需求

下列场景使用 Redis 更好：

- 存储数据量小，内存可以满足
- 业务中大部分使用复合类型，如 Set, Zset, Map, List 类型，但是整体数据量又不大的
