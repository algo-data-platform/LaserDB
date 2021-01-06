package service_router

import (
	"errors"
	"net"
	"strconv"
	"sync"
	"time"

	"github.com/facebook/fbthrift/thrift/lib/go/thrift"
)

type ThriftTransportType string

const (
	THRIFT_TRANSPORT_HEADER  ThriftTransportType = "HEADER"
	THRIFT_TRANSPORT_FREMED  ThriftTransportType = "FRAMED"
	THRIFT_TRANSPORT_HTTP2   ThriftTransportType = "HTTP2"   // not implemented
	THRIFT_TRANSPORT_RSOCKET ThriftTransportType = "RSOCKET" // not implemented
)

type ThriftCompressionMethod uint32

const (
	ThriftCompressionMethod_None   ThriftCompressionMethod = 0
	ThriftCompressionMethod_Zlib   ThriftCompressionMethod = 1
	ThriftCompressionMethod_HMAC   ThriftCompressionMethod = 2
	ThriftCompressionMethod_Snappy ThriftCompressionMethod = 3
	ThriftCompressionMethod_QLZ    ThriftCompressionMethod = 3
	ThriftCompressionMethod_Zstd   ThriftCompressionMethod = 3
)

type ThriftClient interface{}

type (
	NewThriftClient   func(thrift.Transport, thrift.ProtocolFactory) ThriftClient
	CloseThriftClient func(client ThriftClient) error
	ThriftIsOpen      func(client ThriftClient) bool
)

type ConnGroup struct {
	sync.RWMutex
	transport map[string]*Pool
}

// unthreadsafe
// 连接池不会把同一个连接同时分配给两个协程，
// 但是，在获取连接后，不要并发操作连接
type ThriftConn struct {
	sync.Mutex
	err       error
	client    ThriftClient
	closeFunc CloseThriftClient
	pingFunc  ThriftIsOpen
}

func (tc *ThriftConn) Err() error {
	tc.Lock()
	err := tc.err
	tc.Unlock()
	return err
}

func (tc *ThriftConn) SetErr(err error) {
	tc.Lock()
	tc.err = err
	tc.Unlock()
}

func (tc *ThriftConn) ForceClose() bool {
	err := tc.Err()
	if err != nil {
		// 遇到transport错误，强制关闭连接
		// 如果是业务逻辑错误，则把连接归还连接池
		var transportException thrift.TransportException
		if errors.As(err, &transportException) {
			return true
		}
	}
	return false
}

func (tc *ThriftConn) Do(action Action) *Done {
	if action != nil {
		resp, err := action(tc.client)
		//if err != nil {
		//	tc.err = err
		//}
		return &Done{
			Then: &Then{
				Resp: resp,
				Err:  err,
			},
		}
	} else {
		return &Done{
			Then: &Then{},
		}
	}
}

func (tc *ThriftConn) Good(_ time.Time) bool {
	if tc.pingFunc != nil {
		return tc.pingFunc(tc.client)
	}
	return true
}

func (tc *ThriftConn) Close() error {
	tc.Lock()
	err := tc.err
	if tc.err == nil {
		tc.err = errors.New("thrift closed")
		err = tc.closeFunc(tc.client)
	}
	tc.Unlock()
	return err
}

type ThriftConfig struct {
	Host              string
	Port              int
	TransportType     ThriftTransportType
	CompressionMethod ThriftCompressionMethod
	Timeout           time.Duration
	NewThriftClient   NewThriftClient
	CloseThriftClient CloseThriftClient
	ThriftIsOpen      ThriftIsOpen
}

var (
	connGroupOnce     sync.Once
	connGroupInstance *ConnGroup
)

func GetConnGroup() *ConnGroup {
	connGroupOnce.Do(func() {
		connGroupInstance = &ConnGroup{
			transport: make(map[string]*Pool),
		}
	})
	return connGroupInstance
}

func makeTransport(host string, port int, tTransport ThriftTransportType /*, timeout time.Duration*/, transId thrift.TransformID) (thrift.Transport, error) {
	var trans thrift.Transport
	socket, err := thrift.NewSocket(thrift.SocketAddr(net.JoinHostPort(host, strconv.Itoa(port))) /*, thrift.SocketTimeout(timeout)*/)
	if err != nil {
		return nil, err
	}
	if tTransport == THRIFT_TRANSPORT_FREMED {
		trans = thrift.NewFramedTransport(socket)
	} else {
		trans = thrift.NewHeaderTransport(socket)
		// pass TransformNone will cause an exception
		// in fact, only TransformZlib is implemented.
		if transId != thrift.TransformNone {
			trans.(*thrift.HeaderTransport).AddTransform(transId)
		}
	}
	if err = trans.Open(); err != nil {
		return nil, err
	}
	socket.Conn().(*net.TCPConn).SetKeepAlive(true)
	return trans, nil
}

func NewThriftConn(config ThriftConfig) (*ThriftConn, error) {
	trans, err := makeTransport(
		config.Host,
		config.Port,
		config.TransportType,
		//config.Timeout,
		thrift.TransformID(config.CompressionMethod))
	if err != nil {
		return nil, err
	}
	var protocol thrift.ProtocolFactory
	// 目前只支持header和framed两种协议
	if config.TransportType == THRIFT_TRANSPORT_FREMED {
		protocol = thrift.NewBinaryProtocolFactoryDefault()
	} else {
		protocol = thrift.NewHeaderProtocolFactory()
	}
	conn := config.NewThriftClient(trans, protocol)
	return &ThriftConn{
		client:    conn,
		closeFunc: config.CloseThriftClient,
		pingFunc:  config.ThriftIsOpen,
	}, nil
}

func (cg *ConnGroup) GetConnection(
	config ThriftConfig,
	opts ...PoolOption,
) (Conn, error) {
	var (
		pool *Pool
		err  error
		ok   bool
		key  string
		conn Conn
	)
	key = config.Host + strconv.Itoa(config.Port)
	cg.RLock()
	pool, ok = cg.transport[key]
	cg.RUnlock()
	if !ok {
		cg.Lock()
		pool, ok = cg.transport[key]
		if !ok {
			pool = NewPool(func() (Conn, error) {
				return NewThriftConn(config)
			}, opts...)
			cg.transport[key] = pool
		}
		cg.Unlock()
	}
	if conn, err = pool.Aquire(); err != nil {
		return nil, err
	}
	return conn, nil
}
