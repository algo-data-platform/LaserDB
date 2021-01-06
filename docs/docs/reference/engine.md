---
id: engine
title: 存储引擎 
sidebar_label: 存储引擎
slug: /reference/engine
---

对于 Laser 存储系统来说，主要的需求读写高性能、存储容量大以及可以支持批量导入。如果实现大容量存储肯定不能只是把数据存放到内存中，为了同时满足大容量存储和高性能读写的需求，最终在它们两个之间进行权衡
选择 RocksDB 作为存储引擎，RocksDB 支持将数据落地到磁盘中可以有效的解决数据的容量问题，相比较内存数据库读写性能肯定要稍微差些，不过这个问题也不是没有补救的办法，比如 RocksDB 实现了 Write Buffer 来提升
写的性能，降低 IO 操作，通过实现 BlockCache 以及布隆过滤器来加速查找速度，可以有效的弥补读写性能问题。通过使用 RocksDB 存储引擎可以轻松的组织实现 TB 级别的高性能存储系统

### 使用

#### 资源共享

在数据分片章节介绍一个数据表存在多个表分区，一个数据分片中可能包含多个不同数据表分区。其中每个表分区真实物理的一一对应一个 RocksDB 实例，一个服务节点会存在多个 RocksDB 实例，RocksDB 通过对 Compaction 线程池、
Block Cache 共享可以实现单进程多实例运行。

默认情况下创建的 RocksDB 实例都会固定开辟一个 Cache 用来做 Block Data 的 Cache, 如果不进行 Cache 共享每个实例都会创建一个 Cache, 随着服务节点中映射的表分区越来越多，使用的内存也线性增长，最终程序由于没有内存
 Core 掉。并且每个实例开固定的 Cache 不一定合理，由于 Cache 使用的是 LRU Cache, 有的数据表有大量的读操作 Cache 持续更新数据，有的数据表仅仅是某个时间内有读操作吧 Cache 占满后就再没有更新，这样严重的浪费
 , 通过共享 Cache 可以有效的解决上述问题，每个服务节点开辟一个大的 Cache, 所有的数据表共享一个 Cache, 可以将内存使用量控制在一个水平上，并且提升 Cache 的利用率。
 
另外当 RocksDB 有写的操作时会首先将数据写到一个内存数据结构中，也称 MemTable。这个结构是从 Write Buffer 中申请内存使用，当 Writer Buffer 慢了就会触发后台 Flush 流程，同样默认情况下每个 RocksDB
实例都会创建一个专属的 Buffer 来提升写性能。为了很好的控制内存使用，我们将 Write Buffer 也进行了共享, Write Buffer 不用重新开辟内存空间，可以复用 BlockCache 的内存空间，当我们创建 WriteBuffer 对象的时候
可以指定在 BlockCache 中创建，这样我们仅需要每个服务节点控制一个 BlockCache 总体大小就是对应的内存的使用量

最终共享 BlockCache 和 Writer Buffer 方法如下
 
```
// config 是服务节点的配置对象
// 计算 cache 的总体大小，与 write buffer 总体大小
uint64_t cache_size = static_cast<uint64_t>(config->getBlockCacheSizeGb()) * 1024 * 1024 * 1024;
uint64_t write_buffer_size = static_cast<uint64_t>(config->getWriteBufferSizeGb()) * 1024 * 1024 * 1024;
int32_t num_shard_bits = config->getNumShardBits();
bool strict_capacity_limit = config->getStrictCapacityLimit();
double high_pri_pool_ratio = config->getHighPriPoolRatio();
auto cache = rocksdb::NewLRUCache(cache_size, num_shard_bits, strict_capacity_limit, high_pri_pool_ratio);
LOG(INFO) << "Rocksdb block cache capacity size:" << cache_->GetCapacity() << " num shard bits:" << num_shard_bits
          << " strict_capacity_limit:" << strict_capacity_limit << " high_pri_pool_ratio:" << high_pri_pool_ratio;

// write buffer 使用 block cache 的物理内存
auto write_buffer_manager = std::make_shared<rocksdb::WriteBufferManager>(write_buffer_size, cache_);

// 构建 RocksDB::Options
rocksdb::Options options;
rocksdb::BlockBasedTableOptions table_options;
....
table_options.block_cache = cache;
options.write_buffer_manager = write_buffer_manager;
....
```

除了内存资源，对于线程资源 RocksDb 会创建一个单例的 Env 对象， Env 除了适配各种运行环境以外，还负责维护 RocksDB 后台的线程池，所有的 `rocksdb::Options` 创建都会使用默认的 Env 对象，意味着所有的
 RocksDB 实例都是用同一个后台处理线程池，这样有效的降低线程数目，减少线程频繁切换的额外开销。当然根据实际的处理情况可以修改线程池的大小通过 `Env::SetBackgroundThreads` 接口。处理后台用于 Flush, Compaction 操作的
 线程池外，每个 RocksDB 实例都需要 `rocksdb::SstFileManager` 来管理 SST 文件，每个对象中有一个独立的调度线程，这样在每次创建实例都会产生一个线程，随着实例数增加，线程数也在增加，Laser 通过所有实例共享 `rocksdb::SstFileManager`
 对象的方式接受额外的线程创建，具体的方法是：
 
 ```
rocksdb::Option option;
auto sst_file_manager = std::shared_ptr<rocksdb::SstFileManager>(rocksdb::NewSstFileManager(option.env, option.info_log));
...
 option.sst_file_manager = sst_file_manager;
...
```

通过共享 SST 文件管理器可以避免创建大量的线程，不过在 RocksDB 还有两个额外的线程没有办法共享，在 RocksDB 实例创建成功后会通过 `DBImpl::StartTimedTasks` 分别创建 Dump 和持久化统计信息的两个线程，
这两个线程的工作就是定时将统计信息 dump 或者持久化, 对于多实例 RocksDB 统计信息通过在上层统一调用接口来实现统计信息获取，所以我们对 RocksDB 源码进行改造，去掉这两个定时任务，从而使得 RocksDB 实例自己不会额外
创建线程，单个服务节点的线程不会随着承载数据表个数的增多而增多。

| 配置名称 | 默认值 | 说明 |
| :-- | :-- | :-- |
| BlockCacheSizeGb |  2  | 服务节点最大允许的内存缓存大小, 建议设置为系统内存 2/3 |
| HighPriPoolRatio |  0.5  | 缓存池中高优先的 cache 比例 |
| StrictCapacityLimit | true  | 是否强制限制内存大小 |
| NumShardBits | 6  | Cache 分片使用的前缀位数，代表分片的个数 |
| WriteBufferSizeGb | 1  | 写 Buffer 最大大小 |

一般情况下只需要根据系统内存的大小调整 `BlockCacheSizeGb` 和 `WriteBufferSizeGb` 大小即可，其他参数可以不做调整

#### Key 自动过期

#### 配置热更新


### 主要参数

