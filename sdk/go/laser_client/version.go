package laser_client

import "fmt"

const (
	MAJOR_VERSION = 1
	MINOR_VERSION = 0
	REVISION      = 18
)

func Version() string {
	return fmt.Sprintf("%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, REVISION)
}
