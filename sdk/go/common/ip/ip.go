package ip

import (
	"net"
	"time"
)

var (
	localAddrStr string
)

func GetLocalIPv4Str() (string, error) {
	if localAddrStr != "" {
		return localAddrStr, nil
	}
	conn, err := net.DialTimeout("udp", "10.255.255.255:80", 10*time.Millisecond)
	if err != nil {
		return "", err
	}
	defer conn.Close()

	localAddr := conn.LocalAddr().(*net.UDPAddr)
	localAddrStr = localAddr.IP.To4().String()
	return localAddrStr, nil
}

func IsValidAddress(network, address string) bool {
	if len(address) == 0 {
		return false
	}
	conn, err := net.DialTimeout(network, address, 100*time.Millisecond)
	if err != nil {
		return false
	}
	conn.Close()
	return true
}
