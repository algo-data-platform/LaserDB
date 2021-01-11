module github.com/algo-data-platform/LaserDB/sdk/go/laser_client

go 1.15

replace (
	github.com/algo-data-platform/LaserDB/sdk/go/common/ip => ../common/ip
	github.com/algo-data-platform/LaserDB/sdk/go/common/service_router => ../common/service_router
)

require (
	github.com/algo-data-platform/LaserDB/sdk/go/common/ip v0.0.0-20210111033758-ba96f4d1f4a4
	github.com/algo-data-platform/LaserDB/sdk/go/common/service_router v0.0.0-20210111033758-ba96f4d1f4a4
	github.com/armon/go-metrics v0.3.6 // indirect
	github.com/facebook/fbthrift v0.31.0
	github.com/fatih/color v1.10.0 // indirect
	github.com/hashicorp/consul/api v1.8.1
	github.com/hashicorp/go-hclog v0.15.0 // indirect
	github.com/hashicorp/go-immutable-radix v1.3.0 // indirect
	github.com/hashicorp/golang-lru v0.5.4 // indirect
	github.com/liubang/tally v0.0.0-20201228094058-952b6f95b269
	github.com/mitchellh/mapstructure v1.4.0 // indirect
	github.com/rcrowley/go-metrics v0.0.0-20201227073835-cf1acfcdf475 // indirect
	github.com/stretchr/testify v1.6.1
	github.com/zentures/cityhash v0.0.0-20131128155616-cdd6a94144ab
	golang.org/x/sys v0.0.0-20210110051926-789bb1bd4061 // indirect
	google.golang.org/protobuf v1.25.0 // indirect
)
