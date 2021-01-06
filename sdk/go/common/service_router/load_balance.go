package service_router

import (
	"crypto/rand"
	"math"
	"math/big"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
)

func IpToInt(ip string) uint32 {
	var (
		arr   []string
		i     float64 = 3
		total int32   = 0
	)
	arr = strings.Split(ip, ".")
	if len(arr) != 4 {
		return 0
	}
	for _, v := range arr {
		t, _ := strconv.Atoi(v)
		total += int32(float64(t) * math.Pow(256, i))
		i--
	}
	return uint32(total)
}

// LoadBalance interface
type LoadBalance interface {
	Select(serverList ServerList) (*Server, bool)
}

// LoadBalanceRandom
type LoadBalanceRandom struct {
}

func NewLoadBalanceRandom() *LoadBalanceRandom {
	return &LoadBalanceRandom{}
}

func (lbr *LoadBalanceRandom) Select(serverList ServerList) (*Server, bool) {
	if nil == serverList {
		return nil, false
	}
	length := len(serverList)
	if length == 0 {
		return nil, false
	}
	idx, _ := rand.Int(rand.Reader, big.NewInt(int64(length)))
	return &serverList[idx.Int64()], true
}

// LoadBalanceRoundrobin
type LoadBalanceRoundrobin struct {
	request uint64
}

func NewLoadBalanceRoundrobin() *LoadBalanceRoundrobin {
	return &LoadBalanceRoundrobin{
		request: 0,
	}
}

func (lbrr *LoadBalanceRoundrobin) Select(serverList ServerList) (*Server, bool) {
	if nil == serverList {
		return nil, false
	}
	length := len(serverList)
	if length == 0 {
		return nil, false
	}
	req := atomic.AddUint64(&lbrr.request, 1)
	return &serverList[req%uint64(length)], true
}

// LoadBalanceLocalFirst
type LoadBalanceLocalFirst struct {
	localIp uint32
	config  BalanceLocalFirstConfig
}

func NewLoadBalanceLocalFirst(config BalanceLocalFirstConfig) *LoadBalanceLocalFirst {
	return &LoadBalanceLocalFirst{
		config:  config,
		localIp: IpToInt(config.LocalIp),
	}
}

func (lblf *LoadBalanceLocalFirst) Select(serverList ServerList) (*Server, bool) {
	if nil == serverList {
		return nil, false
	}
	var (
		refers    ServerList
		diffrange float64 = float64(lblf.config.DiffRange)
		slength   int     = len(serverList)
	)
	if slength == 0 {
		return nil, false
	}
	for idx := range serverList {
		server := &serverList[idx]
		ip := IpToInt(server.Host)
		if ip == 0 {
			continue
		}

		// 本地ip直接返回
		if ip == lblf.localIp {
			return server, true
		}

		// 同机房机器
		diff := int32(ip - lblf.localIp)
		if math.Abs(float64(diff)) <= diffrange {
			refers = append(refers, *server)
		}
	}
	length := len(refers)
	if length == 0 {
		// 实在没有，就走随机策略
		length = slength
		refers = serverList
	}
	idx, _ := rand.Int(rand.Reader, big.NewInt(int64(length)))
	return &refers[idx.Int64()], true
}

// LoadBalanceIpRangeFirst
type LoadBalanceIpRangeFirst struct {
	localIp uint32
	config  BalanceLocalFirstConfig
}

func NewLoadBalanceIpRangeFirst(config BalanceLocalFirstConfig) *LoadBalanceIpRangeFirst {
	return &LoadBalanceIpRangeFirst{
		config:  config,
		localIp: IpToInt(config.LocalIp),
	}
}

func (lbirf *LoadBalanceIpRangeFirst) Select(serverList ServerList) (*Server, bool) {
	if nil == serverList {
		return nil, false
	}
	var (
		refers    ServerList
		diffrange float64 = float64(lbirf.config.DiffRange)
		slength   int     = len(serverList)
	)
	if slength == 0 {
		return nil, false
	}
	for idx := range serverList {
		server := &serverList[idx]
		ip := IpToInt(server.Host)
		if ip == 0 {
			continue
		}
		// 同机房机器
		diff := ip - lbirf.localIp
		if math.Abs(float64(diff)) <= diffrange {
			refers = append(refers, *server)
		}
	}
	length := len(refers)
	if length == 0 {
		length = slength
		refers = serverList
	}
	idx, _ := rand.Int(rand.Reader, big.NewInt(int64(length)))
	return &refers[idx.Int64()], true
}

// LoadBalanceStaticWeight
type LoadBalanceStaticWeight struct {
}

func NewLoadBalanceStaticWeight() *LoadBalanceStaticWeight {
	return &LoadBalanceStaticWeight{}
}

func (lbsw *LoadBalanceStaticWeight) Select(serverList ServerList) (*Server, bool) {
	if nil == serverList {
		return nil, false
	}
	var (
		weightSum uint32 = 0
		length    int    = len(serverList)
	)
	if length == 0 {
		return nil, false
	}
	for _, server := range serverList {
		weightSum += server.Weight
	}
	if 0 == weightSum {
		idx, _ := rand.Int(rand.Reader, big.NewInt(int64(length)))
		return &serverList[idx.Int64()], true
	}
	n32, _ := rand.Prime(rand.Reader, 32)
	idx := n32.Uint64() % uint64(weightSum)
	for _, server := range serverList {
		if idx < uint64(server.Weight) {
			return &server, true
		}
		idx -= uint64(server.Weight)
	}
	return nil, false
}

// LoadBalanceConfigurableWeight
type LoadBalanceConfigurableWeight struct {
	sync.Mutex
	currentState *CurrentStateWrapper
}

type CurrentStateWrapper struct {
	currentIndex  int64
	currentWeight int64
}

func NewLoadBalanceConfigurableWeight() *LoadBalanceConfigurableWeight {
	return &LoadBalanceConfigurableWeight{
		currentState: &CurrentStateWrapper{
			currentIndex:  -1,
			currentWeight: 0,
		},
	}
}

// 最大公约数
func Gcd(a uint32, b uint32) uint32 {
	if (a % b) == 0 {
		return b
	} else {
		return Gcd(b, a%b)
	}
}

func max(a uint32, b uint32) uint32 {
	if a > b {
		return a
	} else {
		return b
	}
}

func (bcw *LoadBalanceConfigurableWeight) Select(serverList ServerList) (*Server, bool) {
	bcw.Lock()
	defer bcw.Unlock()
	if nil == serverList {
		return nil, false
	}
	var (
		length    int    = len(serverList)
		gcdWeight uint32 = 0
		maxWeight uint32 = 0
		index     int    = 0
	)
	if length == 0 {
		return nil, false
	}
	for index = 0; index < length-1; index++ {
		maxWeight = max(maxWeight, serverList[index].Weight)
		if index == 0 {
			gcdWeight = Gcd(serverList[index].Weight, serverList[index+1].Weight)
		} else {
			gcdWeight = Gcd(gcdWeight, serverList[index].Weight)
		}
	}
	maxWeight = max(maxWeight, serverList[length-1].Weight)
	for {
		state := bcw.currentState
		state.currentIndex = (state.currentIndex + 1) % int64(length)
		if state.currentIndex == 0 {
			state.currentWeight = state.currentWeight - int64(gcdWeight)
			if state.currentWeight <= 0 {
				state.currentWeight = int64(maxWeight)
			}
			if state.currentWeight == 0 {
				return nil, false
			}
		}
		if serverList[state.currentIndex].Weight >= uint32(state.currentWeight) {
			return &serverList[state.currentIndex], true
		}
	}
}
