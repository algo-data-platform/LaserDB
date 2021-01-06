package service_router

import (
	"container/list"
	"errors"
	"io"
	"runtime"
	"sync"
	"sync/atomic"
	"time"

	"github.com/liubang/tally"
)

const (
	defaultMaxIdleTime = time.Duration(30) * time.Second
)

var nowFunc = time.Now

var (
	ErrPoolExhausted = errors.New("connection pool exhausted")
)

type (
	Action    func(conn interface{}) (interface{}, error)
	OnSuccess func(resp interface{})
	OnError   func(err error)
	DoneFunc  func()
)

type Then struct {
	Resp interface{}
	Err  error
}

func (t *Then) OnSuccess(suc OnSuccess) *Then {
	if t.Err == nil && suc != nil {
		suc(t.Resp)
	}
	return t
}

func (t *Then) OnError(err OnError) *Then {
	if t.Err != nil && err != nil {
		err(t.Err)
	}
	return t
}

func (t *Then) TryResponse(suc OnSuccess, err OnError) {
	if t.Err != nil {
		if err != nil {
			err(t.Err)
		}
	} else {
		if suc != nil {
			suc(t.Resp)
		}
	}
}

type Done struct {
	Then *Then
}

func (d *Done) Done(done DoneFunc) *Then {
	if done != nil {
		done()
	}
	return d.Then
}

type Conn interface {
	io.Closer
	Err() error            // 获取错误信息
	Good(t time.Time) bool // 在获取连接的时候调用
	SetErr(err error)      // 在出现错误的时候调用
	ForceClose() bool      // 在释放连接前调用
	Do(action Action) *Done
}

type poolConn struct {
	conn       Conn
	activeTime time.Time
}

type Pool struct {
	sync.Mutex
	Factory      func() (Conn, error)
	MaxIdle      int // Maximum number of idle connections in the pool
	MaxActive    int // Maximum number of connections allocated by the pool, MaxActive >= MaxIdle
	IdleTimeout  time.Duration
	Wait         bool
	Metric       tally.Scope
	initialized  uint32
	closed       bool
	active       int // the number of open connections in the pool, active = idle.Len() + the number of connections in use
	queue        chan struct{}
	idle         *list.List
	blockMeter   tally.Meter
	tcpConnTimer tally.Timer
	//idleGauge    tally.Gauge
	//activeGauge  tally.Gauge
}

type poolOptions struct {
	maxIdle     int
	maxActive   int
	wait        bool
	idleTimeout time.Duration
	metric      tally.Scope
}

type PoolOption func(*poolOptions)

func PoolMetric(m tally.Scope) PoolOption {
	return func(o *poolOptions) {
		o.metric = m
	}
}

func PoolMaxIdle(n int) PoolOption {
	return func(o *poolOptions) {
		o.maxIdle = n
	}
}

func PoolMaxActive(n int) PoolOption {
	return func(o *poolOptions) {
		o.maxActive = n
	}
}

func PoolWait(b bool) PoolOption {
	return func(o *poolOptions) {
		o.wait = b
	}
}

func PoolIdleTimeout(t time.Duration) PoolOption {
	return func(o *poolOptions) {
		o.idleTimeout = t
	}
}

func newPoolOptions(opt ...PoolOption) poolOptions {
	opts := poolOptions{}
	for _, o := range opt {
		o(&opts)
	}
	if opts.maxActive <= 0 {
		opts.maxActive = runtime.NumCPU()
	}
	if opts.maxIdle <= 0 || opts.maxIdle > opts.maxActive {
		opts.maxIdle = opts.maxActive
	}
	if opts.idleTimeout <= 0 {
		opts.idleTimeout = defaultMaxIdleTime
	}
	return opts
}

func NewPool(factory func() (Conn, error), opt ...PoolOption) *Pool {
	opts := newPoolOptions(opt...)
	pool := &Pool{
		Factory:     factory,
		MaxIdle:     opts.maxIdle,
		MaxActive:   opts.maxActive,
		Wait:        opts.wait,
		IdleTimeout: opts.idleTimeout,
		Metric:      opts.metric,
		idle:        list.New(),
	}
	if opts.metric != nil {
		pool.blockMeter = opts.metric.Meter(POOL_METRICS_AQUIRE_BLOCKED)
		//pool.activeGauge = opts.metric.Gauge(POOL_ACTIVE_CONNECTIONS)
		//pool.idleGauge = opts.metric.Gauge(POOL_IDLE_CONNECTIONS)
	}
	return pool
}

func (p *Pool) Close() error {
	p.Lock()
	if p.closed {
		p.Unlock()
		return nil
	}
	p.closed = true
	p.active -= p.idle.Len()
	// truncate idle list and close all idle connections.
	for p.idle.Front() != nil {
		p.idle.Front().Value.(*poolConn).conn.Close()
		p.idle.Remove(p.idle.Front())
	}
	if p.queue != nil {
		close(p.queue)
	}
	p.Unlock()
	return nil
}

func (p *Pool) initQueue() {
	if atomic.LoadUint32(&p.initialized) == 1 {
		return
	}
	p.Lock()
	if p.initialized == 0 {
		if !p.closed {
			p.queue = make(chan struct{}, p.MaxActive)
			for i := 0; i < p.MaxActive; i++ {
				p.queue <- struct{}{}
			}
		}
		atomic.StoreUint32(&p.initialized, 1)
	}
	p.Unlock()
}

func (p *Pool) release(pc *poolConn, err error, forceClose bool) error {
	p.Lock()
	if !p.closed && !forceClose {
		pc.activeTime = nowFunc()
		if err != nil {
			p.idle.PushBack(pc)
		} else {
			p.idle.PushFront(pc)
		}
		if p.idle.Len() > p.MaxIdle {
			pc = p.getBackValue()
			p.idle.Remove(p.idle.Back())
		} else {
			pc = nil
		}
	}

	if pc != nil {
		p.Unlock()
		pc.conn.Close()
		p.Lock()
		p.active--
	}

	// add to wait queue
	if p.queue != nil && !p.closed {
		p.queue <- struct{}{}
	}

	p.Unlock()
	return nil
}

func (p *Pool) Aquire() (Conn, error) {
	pc, err := p.aquire()
	if err != nil {
		return nil, err
	}
	pc.conn.SetErr(nil)
	return &activeConn{pool: p, poolConn: pc}, err
}

func (p *Pool) getFrontValue() *poolConn {
	if p.idle.Front() != nil {
		return p.idle.Front().Value.(*poolConn)
	}
	return nil
}

func (p *Pool) getBackValue() *poolConn {
	if p.idle.Back() != nil {
		return p.idle.Back().Value.(*poolConn)
	}
	return nil
}

func (p *Pool) idleBackTimeout() bool {
	pc := p.getBackValue()
	return pc != nil && pc.activeTime.Add(p.IdleTimeout).Before(nowFunc())
}

func (p *Pool) aquire() (*poolConn, error) {
	if p.Wait && p.MaxActive > 0 {
		p.initQueue()
		if p.blockMeter != nil && len(p.queue) == 0 {
			// mark blocking aquire
			p.blockMeter.Mark(1)
		}
		<-p.queue
	}
	p.Lock()
	// if p.Metric != nil {
	// 	p.activeGauge.Update(float64(p.active))
	// 	p.idleGauge.Update(float64(p.idle.Len()))
	// }
	// prune stale connections at the back of the idle list.
	if p.IdleTimeout > 0 {
		n := p.idle.Len()
		for i := 0; i < n && p.idleBackTimeout(); i++ {
			pc := p.getBackValue()
			p.idle.Remove(p.idle.Back())
			p.Unlock()
			pc.conn.Close()
			p.Lock()
			p.active--
		}
	}

	// get idle connection from the front of idle list.
	for p.idle.Front() != nil {
		pc := p.getFrontValue()
		p.idle.Remove(p.idle.Front())
		p.Unlock()
		if pc.conn.Good(pc.activeTime) {
			// return
			return pc, nil
		}
		// unhealthy, drop
		pc.conn.Close()
		p.Lock()
		p.active--
	}

	if !p.Wait && p.MaxActive > 0 && p.active >= p.MaxActive {
		p.Unlock()
		return nil, ErrPoolExhausted
	}

	p.active++
	p.Unlock()
	c, err := p.factory()
	if err != nil {
		c = nil
		p.Lock()
		p.active--
		// only when p.Wait && p.MaxActive > 0, p.ch whill be initialized.
		if p.queue != nil && !p.closed {
			p.queue <- struct{}{}
		}
		p.Unlock()
		return nil, err
	}
	return &poolConn{conn: c}, nil
}

func (p *Pool) factory() (Conn, error) {
	if p.Factory != nil {
		return p.Factory()
	}
	return nil, errors.New("Factory method must be specified.")
}

type activeConn struct {
	poolConn *poolConn
	pool     *Pool
}

func (ac *activeConn) Close() error {
	pc := ac.poolConn
	if pc == nil {
		return nil
	}
	return ac.pool.release(pc, pc.conn.Err(), pc.conn.ForceClose())
}

func (ac *activeConn) SetErr(err error) {
	pc := ac.poolConn
	if pc == nil {
		return
	}
	pc.conn.SetErr(err)
}

func (ac *activeConn) Err() error {
	pc := ac.poolConn
	if pc == nil {
		return nil
	}
	return pc.conn.Err()
}

func (ac *activeConn) Good(t time.Time) bool {
	pc := ac.poolConn
	if pc == nil {
		return false
	}
	return pc.conn.Good(t)
}

func (ac *activeConn) Do(action Action) *Done {
	pc := ac.poolConn
	if pc == nil {
		return &Done{}
	}
	return pc.conn.Do(action)
}

func (ac *activeConn) ForceClose() bool {
	pc := ac.poolConn
	if pc == nil {
		return true
	}
	return pc.conn.ForceClose()
}
