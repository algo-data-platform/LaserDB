package service_router

import (
	"errors"
	"fmt"
	"testing"
	"time"

	"github.com/facebook/fbthrift/thrift/lib/go/thrift"
)

type testThriftClient struct {
	trans thrift.Transport
	proto thrift.ProtocolFactory
}

func (t *testThriftClient) say() {
	//fmt.Println("hello world")
}

func BenchmarkGetConnection(b *testing.B) {
	cg := GetConnGroup()
	config := ThriftConfig{
		Host:          "www.weibo.com",
		Port:          80,
		TransportType: THRIFT_TRANSPORT_HEADER,
		Timeout:       time.Millisecond * 3,
		NewThriftClient: func(socket thrift.Transport, p thrift.ProtocolFactory) ThriftClient {
			return &testThriftClient{
				trans: socket,
				proto: p,
			}
		},
		CloseThriftClient: func(conn ThriftClient) error {
			return conn.(*testThriftClient).trans.Close()
		},
		ThriftIsOpen: func(conn ThriftClient) bool {
			return conn.(*testThriftClient).trans.IsOpen()
		},
	}
	f := func() {
		conn, err := cg.GetConnection(
			config,
			PoolMaxActive(2), PoolMaxIdle(2), PoolWait(true),
		)
		if err == nil {
			conn.Do(func(c interface{}) (interface{}, error) {
				c.(*testThriftClient).say()
				return nil, errors.New("test")
			})
			conn.Close()
		} else {
			fmt.Println(err)
		}
	}
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		go f()
		go f()
		go f()
		go f()
		go f()
		go f()
		go f()
		go f()
		go f()
	}
}
