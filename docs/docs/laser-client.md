---
id: laser-client
title: Laser 客户端
sidebar_label: Laser 客户端
---

目前 Laser 提供了两种官方 SDK，一个是 C++版本，另一个是 Go 版本。这两种 SDK 都是直接以 fbthrift rpc
client 的形式来请求 LaserDB，相对于通过 redis client 访问 Laser
Proxy 的形式，性能更好，因此对性能要求比较苛刻的服务建议直接使用 SDK。

## C++ client

```cpp
int main(int argc, char *argv[]) {
    FLAGS_logtostderr = true;
    folly::Init init(&argc, &argv);
    // 这里只需要传入LaserServer集群的service name即可
    auto client = std::make_unique<laser::LaserClient>("laser_online");
    // 初始化client
    client->init();

    laser::ClientOption option;
    // 设置超时时间 (ms)
    option.setReceiveTimeoutMs(100);
    // 设置读模式：
    //   MIXED_READ: 混合读模式，既会读主，也可能读从
    //   LEADER_READ: 只读主
    //   FOLLOWER_READ: 只读从
    option.setReadMode(laser::ClientRequestReadMode::MIXED_READ);

    laser::LaserKV kv;
    laser::LaserKey laser_key;
    // 设置库名
    laser_key.set_database_name("dbname");
    // 设置表名
    laser_key.set_table_name("tbname");
    // 设置主键
    laser_key.set_primary_keys({"key"});
    laser::LaserValue laser_value;
    // value
    laser_value.set_string_value("value");
    kv.set_key(laser_key);
    kv.set_value(laser_value);
    auto ret = client->setSync(option, kv);
    LOG(INFO) << "test set ret: " << ret;

    ret = client->getSync(option, &value, laser_key);
    LOG(INFO) << "test get ret: " << ret << ",value: " << value;

    return 0;
}
```

## Go client

```go
import (
	"log"

	"github.com/algo-data-platform/LaserDB/sdk/go/common/service_router"
	"github.com/algo-data-platform/LaserDB/sdk/go/laser_client"
	"github.com/algo-data-platform/LaserDB/sdk/go/laser_client/if/laser"
	"github.com/hashicorp/consul/api"
)

func main() {
	client := laser_client.NewLaserClient(&laser_client.ClientConfig{
		Consul: api.Config{
			Address: "127.0.0.1:1234", // 设置LaserServer集群使用的Consul的地址
		},
		LaserServiceName: "laser_online", // LaserServer集群服务名称
		ProjectName:      "demo",         // 客户端名称，metrics中会用来区分客户端
	})
	options := laser_client.ClientOption{
		MaxConnPerServer:        4,                                           // 连接池配置
		ConnIdleTimeoutMs:       5,                                           // tcp 连接超时时间
		ReceiveTimeoutMs:        5,                                           // tcp 接收数据超时时间
		ConnectionRetry:         3,                                           // tcp 连接超时重试次数，发生在连接建立的时候
		TimeoutRetry:            3,                                           // tcp 请求连接超时重试次数，作用于rpc请求发生的时候
		ThriftTransport:         service_router.THRIFT_TRANSPORT_HEADER,      // thrift transport
		ThriftCompressionMethod: service_router.ThriftCompressionMethod_None, // thrift rpc 压缩算法
		LoadBalance:             service_router.LoadBalanceMethod_ROUNDROBIN, // 负载均衡策略
		ReadMode:                laser_client.MIXED_READ,                     // 读模式
	}

	value := "value"
	status := client.SetSync(options, laser.LaserKV{
		Key: &laser.LaserKey{
			DatabaseName: "dbname",
			TableName:    "tbname",
			PrimaryKeys:  []string{"key"},
		},
		Value: &laser.LaserValue{
			StringValue: &value,
		},
	})
	log.Println(status.String())

	status, ret := client.GetSync(options, laser.LaserKey{
		DatabaseName: "dbname",
		TableName:    "tbname",
		PrimaryKeys:  []string{"key"},
	})
	log.Println(status.String())
	log.Println(ret)
}
```
