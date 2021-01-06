package service_router

import (
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

var serverList = ServerList{
	Server{
		Host:        "192.168.1.1",
		Port:        8888,
		Protocol:    ServerProtocol_THRIFT,
		Weight:      10,
		ServiceName: "test",
		UpdateTime:  uint64(time.Now().UnixNano() / 1e6),
		Status:      ServerStatus_AVAILABLE,
	},
	Server{
		Host:        "192.168.1.2",
		Port:        9999,
		Protocol:    ServerProtocol_THRIFT,
		Weight:      1,
		ServiceName: "test",
		UpdateTime:  uint64(time.Now().UnixNano() / 1e6),
		Status:      ServerStatus_AVAILABLE,
	},
	Server{
		Host:        "192.168.1.3",
		Port:        6666,
		Protocol:    ServerProtocol_THRIFT,
		Weight:      8,
		ServiceName: "test",
		UpdateTime:  uint64(time.Now().UnixNano() / 1e6),
		Status:      ServerStatus_AVAILABLE,
	},
	Server{
		Host:        "192.168.10.1",
		Port:        8888,
		Protocol:    ServerProtocol_THRIFT,
		Weight:      10,
		ServiceName: "test",
		UpdateTime:  uint64(time.Now().UnixNano() / 1e6),
		Status:      ServerStatus_AVAILABLE,
	},
	Server{
		Host:        "192.168.10.2",
		Port:        8080,
		Protocol:    ServerProtocol_THRIFT,
		Weight:      8,
		ServiceName: "test",
		UpdateTime:  uint64(time.Now().UnixNano() / 1e6),
		Status:      ServerStatus_AVAILABLE,
	},
}

func TestIpToInt(t *testing.T) {
	useCase := map[string]uint32{
		"192.168.1.1":   2158493953,
		"10.85.57.206":  173357518,
		"10.131.14.231": 176361191,
	}
	for k, v := range useCase {
		ival := IpToInt(k)
		assert.Equal(t, v, ival, "error")
	}
}

func TestLoadBalanceLocalFirst(t *testing.T) {
	lb := NewLoadBalanceLocalFirst(BalanceLocalFirstConfig{
		LocalIp:   "192.168.1.24",
		DiffRange: 256,
	})

	exp := []string{"192.168.1.1", "192.168.1.2", "192.168.1.3"}
	res := make(map[string]int)
	for i := 0; i < 10000; i++ {
		server, ok := lb.Select(serverList)
		res[server.Host]++
		assert.True(t, ok, "error")
		assert.True(t, strings.Contains(strings.Join(exp, ","), server.Host), "error")
	}
	assert.Equal(t, len(res), 3, "error")

	lb = NewLoadBalanceLocalFirst(BalanceLocalFirstConfig{
		LocalIp:   "192.168.10.123",
		DiffRange: 256,
	})
	exp = []string{"192.168.10.1", "192.168.10.2"}
	res = make(map[string]int)
	for i := 0; i < 10000; i++ {
		server, ok := lb.Select(serverList)
		res[server.Host]++
		assert.True(t, ok, "error")
		assert.True(t, strings.Contains(strings.Join(exp, ","), server.Host), "error")
	}
	assert.Equal(t, len(res), 2, "error")

	lb = NewLoadBalanceLocalFirst(BalanceLocalFirstConfig{
		LocalIp:   "192.168.10.123",
		DiffRange: 65536,
	})
	exp = []string{"192.168.1.1", "192.168.1.2", "192.168.1.3", "192.168.10.1", "192.168.10.2"}
	res = make(map[string]int)
	for i := 0; i < 10000; i++ {
		server, ok := lb.Select(serverList)
		res[server.Host]++
		assert.True(t, ok, "error")
		assert.True(t, strings.Contains(strings.Join(exp, ","), server.Host), "error")
	}
	assert.Equal(t, len(res), 5, "error")
}

func BenchmarkLoadBalanceRandom(b *testing.B) {
	lb := NewLoadBalanceRandom()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = lb.Select(serverList)
	}
}

func BenchmarkLoadBalanceRoundrobin(b *testing.B) {
	lb := NewLoadBalanceRoundrobin()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = lb.Select(serverList)
	}
}

func BenchmarkLoadBalanceLocalFirst(b *testing.B) {
	lb := NewLoadBalanceLocalFirst(BalanceLocalFirstConfig{
		LocalIp:   "192.168.1.1",
		DiffRange: 256,
	})
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = lb.Select(serverList)
	}
}

func BenchmarkLoadBalanceIpRangeFirst(b *testing.B) {
	lb := NewLoadBalanceIpRangeFirst(BalanceLocalFirstConfig{
		LocalIp:   "192.168.1.1",
		DiffRange: 256,
	})
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = lb.Select(serverList)
	}
}

func BenchmarkLoadBalanceConfigurableWeight(b *testing.B) {
	lb := NewLoadBalanceConfigurableWeight()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = lb.Select(serverList)
	}
}

func BenchmarkLoadBalanceStaticWeight(b *testing.B) {
	lb := NewLoadBalanceStaticWeight()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = lb.Select(serverList)
	}
}
