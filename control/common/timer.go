package common

import (
	"time"
)

func InitCleanTokens(interval int32, callback func()) {
	ticker := time.NewTicker(time.Duration(interval) * time.Second)
	go func() {
		for {
			select {
			case <-ticker.C:
				callback()
			}
		}
	}()
}
