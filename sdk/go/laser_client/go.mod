module github.com/algo-data-platform/LaserDB/sdk/go/laser_client

go 1.15

replace (
	github.com/algo-data-platform/LaserDB/sdk/go/common/ip => ../common/ip
	github.com/algo-data-platform/LaserDB/sdk/go/common/service_router => ../common/service_router
)

require (
	github.com/algo-data-platform/LaserDB/sdk/go/common/ip v0.0.0-00010101000000-000000000000
	github.com/algo-data-platform/LaserDB/sdk/go/common/service_router v0.0.0-00010101000000-000000000000
	github.com/facebook/fbthrift v0.31.0
	github.com/hashicorp/consul/api v1.8.1
	github.com/liubang/tally v0.0.0-20201228094058-952b6f95b269
	github.com/stretchr/testify v1.6.1
	github.com/zentures/cityhash v0.0.0-20131128155616-cdd6a94144ab
)
