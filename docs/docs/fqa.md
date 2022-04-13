---
id: fqa
title: 常见问题
sidebar_label: 常见问题
slug: /fqa
---

## 通用

- 什么是 LaserDB?

  LaserDB 是一个高性能在线分布式 KV 数据库，它支持高性能读写、批量导入、批量获取、单集群多租户等特性, 是一个满足机器学习、深度学习或者
  其他业务场景的高性能、大容量 KV 存储数据库。

- LaserDB 和 Redis 关系？

  LaserDB 只是通过 LaserProxy 支持兼容了 Redis 协议，用户可以通过 Redis client 和 LaserProxy 交互完成数据的读写，当然 LaserDB 也提供了
  C++ 和 Golang 的客户端 SDK 可以更加高效的与 LaserServer 交互

- LaserDB 是否支持读写分离？

  LaserDB 在 Laser Client 层上自动进行判断主从，如果是写操作都会在主节点上操作完成，如果是读操作用户可以选择 Leader_read(仅从主节点读取)、Follower_read(仅从从节点上读取) 与 Mixed_read(主从混合读取)
  三种模式，用户可以根据自己的部署情况或者业务需求灵活进行选择，除了 Laser Client 支持读写模式选择外，Laser Proxy 用户也可以灵活的指定
