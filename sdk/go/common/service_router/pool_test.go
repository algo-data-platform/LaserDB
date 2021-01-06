package service_router

import (
	"net"
	"testing"
	"time"

	"github.com/facebook/fbthrift/thrift/lib/go/thrift"
)

type testConn struct {
	conn              ThriftClient
	thriftIsOpen      ThriftIsOpen
	closeThriftClient CloseThriftClient
	t                 time.Time
}

func (tc *testConn) Close() error {
	return tc.closeThriftClient(tc.conn)
}

func (tc *testConn) Good(_ time.Time) bool {
	return true
}

func (tc *testConn) Err() error {
	return nil
}

func (tc *testConn) SetErr(_ error) {
}

func (tc *testConn) Do(_ Action) *Done {
	return &Done{}
}

func (tc *testConn) ForceClose() bool {
	return false
}

func f() (Conn, error) {
	socket, _ := thrift.NewSocket(thrift.SocketAddr(net.JoinHostPort("weibo.com", "80")))
	return &testConn{
		conn: socket,
		thriftIsOpen: func(conn ThriftClient) bool {
			return conn.(*thrift.Socket).IsOpen()
		},
		closeThriftClient: func(conn ThriftClient) error {
			return conn.(*thrift.Socket).Close()
		},
	}, nil
}

func BenchmarkAquire(b *testing.B) {
	pool := NewPool(f, PoolMaxIdle(30), PoolMaxActive(30))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		conn, _ := pool.Aquire()
		conn.Close()
	}
}
