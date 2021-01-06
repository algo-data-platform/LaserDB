package service

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math"
	"net/http"
	"net/url"
	"strconv"
	"sync"
	"time"

	"github.com/robfig/cron/v3"

	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
)

const (
	PromqlPatternNodeCpuUsage         = `100 - avg without(cpu) (irate(node_cpu_seconds_total{mode="idle",services=~".*,laser,.*", instance="%s"}[5m])) * 100`
	PromqlPatternNodeCpuSystem        = `avg without(cpu)(irate(node_cpu_seconds_total{mode="system", services=~".*,laser,.*", instance="%s"}[5m])) * 100`
	PromqlPatternNodeCpuUser          = `avg without(cpu)(irate(node_cpu_seconds_total{mode="user", services=~".*,laser,.*", instance="%s"}[5m])) * 100`
	PromqlPatternNodeDiskSizeTotal    = `sort_desc(max(node_filesystem_size_bytes{services=~".*,laser,.*", instance="%s", mountpoint=~"((/data0|/var|/data1|/data2).*|(/))"}) by (mountpoint))`
	PromqlPatternNodeDiskSizeFree     = `avg_over_time(node_filesystem_free_bytes{services=~".*,laser,.*", instance="%s", fstype=~"ext4|xfs", mountpoint="%s"}[5m])`
	PromqlPatternNodeMemSizeTotal     = `node_memory_MemTotal_bytes{services=~".*,laser,.*", instance="%s"}`
	PromqlPatternNodeMemSizeAvailable = `avg_over_time(node_memory_MemAvailable_bytes{services=~".*,laser,.*", instance="%s"}[5m])`
	PromqlPatternNodeNetIn            = `sum(rate(node_network_receive_bytes_total{services=~".*,laser,.*",device!="lo", instance="%s"}[5m]))`
	PromqlPatternNodeNetOut           = `sum(rate(node_network_transmit_bytes_total{services=~".*,laser,.*",device!="lo", instance="%s"}[5m]))`
	PromqlPatternNodeIOUsageRate      = `max(rate(node_disk_io_time_seconds_total{instance="%s", services=~".*,laser,.*"}[5m]) * 100)`
	PromqlPatternNodeQps              = `avg_over_time(ad_core_laser_service_metric_rpc_times{instance="%s:%d", timers_type="min_1"}[5m])`
	PromqlPatternNodeReadKps          = `sum(avg_over_time(ad_core_rocksdb_table_rocksdb_read_kps_min_1{instance="%s:%d"}[5m]))`
	PromqlPatternNodeWriteKps         = `sum(avg_over_time(ad_core_rocksdb_table_rocksdb_write_kps_min_1{instance="%s:%d"}[5m]))`
	PromqlPatternNodeReadBps          = `sum(avg_over_time(ad_core_rocksdb_table_rocksdb_read_bytes_min_1{instance="%s:%d"}[5m]))`
	PromqlPatternNodeWriteBps         = `sum(avg_over_time(ad_core_rocksdb_table_rocksdb_write_bytes_min_1{instance="%s:%d"}[5m]))`
	PromqlPatternNodeP99              = `avg_over_time(ad_core_laser_service_metric_rpc_times{timers_type="percent_0.99", level="60", instance="%s:%d"}[5m])`
	PromqlPatternNodeP999             = `avg_over_time(ad_core_laser_service_metric_rpc_times{timers_type="percent_0.999", level="60", instance="%s:%d"}[5m])`
	PromqlPatternNodeDataSize         = `sum(avg_over_time(ad_core_shard_stat{property="rocksdb.live-sst-files-size", instance="%s:%d"}[5m]))`
	PromqlPatternNodeJitterDuration   = `(avg(count_over_time(ad_core_laser_p999_jitter{instance="%s"}[1d])) by (instance) / avg(count_over_time(ad_core_laser_service_metric_rpc_times{timers_type="percent_0.99", level="60", instance="%s"}[1d])) by (instance)) * 24 * 60 * 60`
	PromqlPatternTableDataSize        = `sum(avg_over_time(ad_core_rocksdb_table_rocksdb_live_sst_files_size{service_name="%s", role="leader", database_name="%s",table_name="%s"}[5m]))`
	PromqlPatternTableReadKps         = `sum(avg_over_time(ad_core_rocksdb_table_rocksdb_read_kps_min_1{service_name="%s", database_name="%s", table_name="%s"}[5m]))`
	PromqlPatternTableWriteKps        = `sum(avg_over_time(ad_core_rocksdb_table_rocksdb_write_kps_min_1{service_name="%s", database_name="%s", table_name="%s"}[5m]))`
	PromqlPatternTableReadBps         = `sum(avg_over_time(ad_core_rocksdb_table_rocksdb_read_bytes_min_1{service_name="%s", database_name="%s", table_name="%s"}[5m]))`
	PromqlPatternTableWriteBps        = `sum(avg_over_time(ad_core_rocksdb_table_rocksdb_write_bytes_min_1{service_name="%s", database_name="%s", table_name="%s"}[5m]))`
)

const (
	RedundancyIndex = 1.5
)

type TimeType int32

const (
	TimeTypePeacePeak TimeType = iota + 1
	TimeTypePeak
)

type PrometheusResponse struct {
	Status string `json:"status"`
	Data   struct {
		ResultType string `json:"resultType"`
		Result     []struct {
			Metric map[string]string `json:"metric"`
			Value  []interface{}     `json:"value"`
		} `json:"result"`
	} `json:"data"`
}

type ReportMetricsCache struct {
	NodePhysicalMetricsCache        sync.Map
	NodeRunningMetricsCache         sync.Map
	GroupRunningMetricsCache        sync.Map
	ResourcePoolRunningMetricsCache sync.Map
}

func NewNodeMetricsCache() *ReportMetricsCache {
	return &ReportMetricsCache{
		NodePhysicalMetricsCache:        sync.Map{},
		NodeRunningMetricsCache:         sync.Map{},
		GroupRunningMetricsCache:        sync.Map{},
		ResourcePoolRunningMetricsCache: sync.Map{},
	}
}

var (
	reportDataCollector     *ReportDataCollector
	reportDataCollectorOnce sync.Once
)

type ReportDataCollector struct {
	ctx  *context.Context
	cron *cron.Cron
}

func GetReportDataCollector(ctx *context.Context) {
	reportDataCollectorOnce.Do(func() {
		cron.New()
		instance := &ReportDataCollector{
			ctx:  ctx,
			cron: cron.New(cron.WithSeconds()),
		}

		cronExpressionPeak := instance.ctx.GetConfig().ReportCronExpressionPeak()
		cronExpressionPeacePeak := instance.ctx.GetConfig().ReportCronExpressionPeacePeak()
		instance.cron.AddFunc(cronExpressionPeak, func() { instance.collectReportData(TimeTypePeak) })
		instance.cron.AddFunc(cronExpressionPeacePeak, func() { instance.collectReportData(TimeTypePeacePeak) })
		instance.cron.Start()
		reportDataCollector = instance
	})
}

func (collector *ReportDataCollector) collectReportData(timeType TimeType) {
	collector.ctx.Log().Info("Report data collection begin...")
	cache := NewNodeMetricsCache()
	collector.getAllNodeMetrics(cache, timeType)
	collector.getAllGroupRunningMetrics(cache, timeType)
	collector.getAllResourcePoolRunningMetrics(cache, timeType)
	collector.getClusterRunningMetrics(cache, timeType)
	collector.getAllTableMetrics(timeType)
	collector.ctx.Log().Info("Report data collection finish.")
}

func (collector *ReportDataCollector) getAllNodeMetrics(cache *ReportMetricsCache, timeType TimeType) {
	nodeManager := GetNodeModel(collector.ctx)
	uniqueNodeIps := make(map[string]bool)
	nodeListParams := params.NodeListParams{}
	nodes, _ := nodeManager.list(nodeListParams, false)
	for _, node := range nodes {
		if _, ok := uniqueNodeIps[node.Host]; !ok {
			uniqueNodeIps[node.Host] = *node.IsEdgeNode
		}
	}

	wg := sync.WaitGroup{}
	wg.Add(len(uniqueNodeIps))
	for nodeIP, isEdgeNode := range uniqueNodeIps {
		copiedIP := nodeIP
		copiedIsEdgeNode := isEdgeNode
		go collector.getNodePhysicalMetrics(copiedIP, copiedIsEdgeNode, cache, timeType, &wg)
	}
	wg.Wait()

	wg.Add(len(nodes))
	for _, node := range nodes {
		host := node.Host
		port := node.Port
		go collector.getNodeRunningMetrics(host, port, cache, timeType, &wg)
	}
	wg.Wait()
}

func (collector *ReportDataCollector) getNodePhysicalMetrics(nodeIP string, isEdgeNode bool,
	cache *ReportMetricsCache, timeType TimeType, wg *sync.WaitGroup) {
	nodePhysicalMetricsModel := NewNodePhysicalMetricsModel(collector.ctx)
	metrics := params.NodePhysicalMetricsInfo{
		ServiceAddress: nodeIP,
		CollectTime:    time.Now().Unix(),
		TimeType:       int32(timeType),
	}
	metrics.CpuUsageRate, metrics.CpuUsageRateSystem, metrics.CpuUsageRateUser, _ = collector.getNodeCpuUsageRate(nodeIP)
	metrics.DiskSizeTotal, metrics.DiskSizeUsage, metrics.DiskUsageRate, _ = collector.getNodeDiskMetrics(nodeIP)
	metrics.MemorySizeTotal, metrics.MemorySizeUsage, metrics.MemoryUsageRate, _ = collector.getNodeMemoryMetrics(nodeIP)
	metrics.IoUsageRate, _ = collector.getNodeIoUsageRate(nodeIP)
	metrics.NetworkIn, metrics.NetworkOut, _ = collector.getNodeNetworkMetrics(nodeIP)

	metrics.CapacityIdleRateAbsolute = float32(100 - math.Max(float64(metrics.CpuUsageRate),
		math.Max(float64(metrics.IoUsageRate), float64(metrics.DiskUsageRate))))
	if isEdgeNode {
		metrics.CapacityIdleRateAbsolute = float32(math.Min(float64(metrics.CapacityIdleRateAbsolute),
			float64(100-metrics.MemoryUsageRate)))
	}
	if status := nodePhysicalMetricsModel.Insert(metrics); !status.Ok() {
		collector.ctx.Log().Error(fmt.Sprintf("Insert node physical metrics of [%s] failed! error: %s",
			metrics.ServiceAddress, status.Error()))
	}
	cache.NodePhysicalMetricsCache.Store(nodeIP, metrics)
	wg.Done()
}

func (collector *ReportDataCollector) getNodeRunningMetrics(nodeIP string, port uint16,
	cache *ReportMetricsCache, timeType TimeType, wg *sync.WaitGroup) {
	nodeRunningMetricsModel := NewNodeRunningMetricsModel(collector.ctx)
	metrics := params.NodeRunningMetricsInfo{
		CollectTime: time.Now().Unix(),
		TimeType:    int32(timeType),
	}
	metrics.ServiceAddress = fmt.Sprintf("%s:%d", nodeIP, port)
	metrics.Qps, _ = collector.getNodeQps(nodeIP, port)
	metrics.KpsRead, metrics.KpsWrite, metrics.Kps, _ = collector.getNodeKps(nodeIP, port)
	metrics.DataSize, _ = collector.getNodeDataSize(nodeIP, port)
	metrics.BpsRead, metrics.BpsWrite, metrics.Bps, _ = collector.getNodeBps(nodeIP, port)
	metrics.P99, metrics.P999, _ = collector.getNodeDelayTime(nodeIP, port)
	metrics.JitterDuration, _ = collector.getJitterDuration(nodeIP, port)

	if mapValue, ok := cache.NodePhysicalMetricsCache.Load(nodeIP); ok {
		if nodePhysicalMetrics, ok := mapValue.(params.NodePhysicalMetricsInfo); ok {
			metrics.CapacityIdleRateAbsolute = nodePhysicalMetrics.CapacityIdleRateAbsolute
			metrics.CapacityIdleRate = nodePhysicalMetrics.CapacityIdleRateAbsolute / RedundancyIndex
		}
	}
	if status := nodeRunningMetricsModel.Insert(metrics); !status.Ok() {
		collector.ctx.Log().Error(fmt.Sprintf("Insert node running metrics of [%s] failed! error: %s",
			metrics.ServiceAddress, status.Error()))
	}
	cache.NodeRunningMetricsCache.Store(metrics.ServiceAddress, metrics)
	wg.Done()
}

func (collector *ReportDataCollector) getAllGroupRunningMetrics(cache *ReportMetricsCache, timeType TimeType) {
	groupRunningMetricsModel := NewGroupRunningMetricsModel(collector.ctx)
	groupModel := NewGroupModel(collector.ctx)
	groupListParams := params.GroupListParams{
		ShowDetails: true,
	}
	groups, _ := groupModel.List(groupListParams, false)
	for _, group := range groups {
		groupRunningMetrics := params.GroupRunningMetricsInfo{
			GroupName:                group.Name,
			CapacityIdleRate:         math.MaxFloat32,
			CapacityIdleRateAbsolute: math.MaxFloat32,
			CollectTime:              time.Now().Unix(),
			TimeType:                 int32(timeType),
		}

		valideNodeCount := 0
		for _, node := range group.Nodes {
			serviceAddress := fmt.Sprintf("%s:%d", node.Host, node.Port)
			mapValue, ok := cache.NodeRunningMetricsCache.Load(serviceAddress)
			if !ok {
				continue
			}
			nodeRunningMetrics, ok := mapValue.(params.NodeRunningMetricsInfo)
			if !ok {
				continue
			}
			groupRunningMetrics.Qps += nodeRunningMetrics.Qps
			groupRunningMetrics.Kps += nodeRunningMetrics.Kps
			groupRunningMetrics.KpsWrite += nodeRunningMetrics.KpsWrite
			groupRunningMetrics.KpsRead += nodeRunningMetrics.KpsRead
			groupRunningMetrics.BpsWrite += nodeRunningMetrics.BpsWrite
			groupRunningMetrics.BpsRead += nodeRunningMetrics.BpsRead
			groupRunningMetrics.DataSize += nodeRunningMetrics.DataSize
			groupRunningMetrics.JitterDuration += nodeRunningMetrics.JitterDuration
			groupRunningMetrics.P99 += nodeRunningMetrics.P99
			groupRunningMetrics.P999 += nodeRunningMetrics.P999
			groupRunningMetrics.CapacityIdleRate = float32(math.Min(float64(groupRunningMetrics.CapacityIdleRate),
				float64(nodeRunningMetrics.CapacityIdleRate)))
			groupRunningMetrics.CapacityIdleRateAbsolute = float32(math.Min(float64(groupRunningMetrics.CapacityIdleRateAbsolute),
				float64(nodeRunningMetrics.CapacityIdleRateAbsolute)))
			valideNodeCount++
		}
		if valideNodeCount == 0 {
			continue
		}
		groupRunningMetrics.Bps = groupRunningMetrics.BpsRead + groupRunningMetrics.BpsWrite
		groupRunningMetrics.P99 /= float32(valideNodeCount)
		groupRunningMetrics.P999 /= float32(valideNodeCount)
		if status := groupRunningMetricsModel.Insert(groupRunningMetrics); !status.Ok() {
			collector.ctx.Log().Error(fmt.Sprintf("Insert group running metrics of [%s] failed! error: %s",
				group.Name, status.Error()))
		}
		cache.GroupRunningMetricsCache.Store(group.Id, groupRunningMetrics)
	}
}

func (collector *ReportDataCollector) getAllResourcePoolRunningMetrics(cache *ReportMetricsCache, timeType TimeType) {
	resourcePoolMerticsModel := NewResourcePoolRunningMetricsModel(collector.ctx)
	dcModel := NewDcModel(collector.ctx)
	dcListParams := params.DcListParams{}
	dcs, _ := dcModel.List(dcListParams, false)
	for _, dc := range dcs {
		resourcePoolRunningMetrics := params.ResourcePoolRunningMetricsInfo{
			PoolName:                 dc.Name,
			CapacityIdleRate:         math.MaxFloat32,
			CapacityIdleRateAbsolute: math.MaxFloat32,
			CollectTime:              time.Now().Unix(),
			TimeType:                 int32(timeType),
		}

		valideGroupCount := 0
		for _, group := range dc.Groups {
			mapValue, ok := cache.GroupRunningMetricsCache.Load(group.Id)
			if !ok {
				continue
			}
			groupRunningMetrics, ok := mapValue.(params.GroupRunningMetricsInfo)
			if !ok {
				continue
			}
			resourcePoolRunningMetrics.Qps += groupRunningMetrics.Qps
			resourcePoolRunningMetrics.Kps += groupRunningMetrics.Kps
			resourcePoolRunningMetrics.KpsWrite += groupRunningMetrics.KpsWrite
			resourcePoolRunningMetrics.KpsRead += groupRunningMetrics.KpsRead
			resourcePoolRunningMetrics.BpsWrite += groupRunningMetrics.BpsWrite
			resourcePoolRunningMetrics.BpsRead += groupRunningMetrics.BpsRead
			resourcePoolRunningMetrics.DataSize += groupRunningMetrics.DataSize
			resourcePoolRunningMetrics.JitterDuration += groupRunningMetrics.JitterDuration
			resourcePoolRunningMetrics.P99 += groupRunningMetrics.P99
			resourcePoolRunningMetrics.P999 += groupRunningMetrics.P999
			resourcePoolRunningMetrics.CapacityIdleRate = float32(math.Min(float64(resourcePoolRunningMetrics.CapacityIdleRate),
				float64(groupRunningMetrics.CapacityIdleRate)))
			resourcePoolRunningMetrics.CapacityIdleRateAbsolute = float32(math.Min(float64(resourcePoolRunningMetrics.CapacityIdleRateAbsolute),
				float64(groupRunningMetrics.CapacityIdleRateAbsolute)))
			valideGroupCount++
		}
		if valideGroupCount == 0 {
			continue
		}
		resourcePoolRunningMetrics.Bps = resourcePoolRunningMetrics.BpsRead + resourcePoolRunningMetrics.BpsWrite
		resourcePoolRunningMetrics.P99 /= float32(valideGroupCount)
		resourcePoolRunningMetrics.P999 /= float32(valideGroupCount)
		if status := resourcePoolMerticsModel.Insert(resourcePoolRunningMetrics); !status.Ok() {
			collector.ctx.Log().Error(fmt.Sprintf("Insert resource pool running metrics of [%s] failed! error: %s",
				dc.Name, status.Error()))
		}
		cache.ResourcePoolRunningMetricsCache.Store(dc.Id, resourcePoolRunningMetrics)
	}
}

func (collector *ReportDataCollector) getClusterRunningMetrics(cache *ReportMetricsCache, timeType TimeType) {
	clusterMerticsModel := NewClusterRunningMetricsModel(collector.ctx)
	clusterModel := NewClusterModel(collector.ctx)
	clusterListParams := params.ClusterListParams{}
	clusters, _ := clusterModel.List(clusterListParams, false)
	for _, cluster := range clusters {
		clusterRunningMetrics := params.ClusterRunningMetricsInfo{
			CapacityIdleRate:         math.MaxFloat32,
			CapacityIdleRateAbsolute: math.MaxFloat32,
			CollectTime:              time.Now().Unix(),
			TimeType:                 int32(timeType),
		}

		valideDcCount := 0
		for _, dc := range cluster.Dcs {
			mapValue, ok := cache.ResourcePoolRunningMetricsCache.Load(dc.Id)
			if !ok {
				continue
			}
			resourcePoolRunningMetrics, ok := mapValue.(params.ResourcePoolRunningMetricsInfo)
			if !ok {
				continue
			}
			clusterRunningMetrics.Qps += resourcePoolRunningMetrics.Qps
			clusterRunningMetrics.Kps += resourcePoolRunningMetrics.Kps
			clusterRunningMetrics.KpsWrite += resourcePoolRunningMetrics.KpsWrite
			clusterRunningMetrics.KpsRead += resourcePoolRunningMetrics.KpsRead
			clusterRunningMetrics.BpsWrite += resourcePoolRunningMetrics.BpsWrite
			clusterRunningMetrics.BpsRead += resourcePoolRunningMetrics.BpsRead
			clusterRunningMetrics.DataSize += resourcePoolRunningMetrics.DataSize
			clusterRunningMetrics.JitterDuration += resourcePoolRunningMetrics.JitterDuration
			clusterRunningMetrics.P99 += resourcePoolRunningMetrics.P99
			clusterRunningMetrics.P999 += resourcePoolRunningMetrics.P999
			clusterRunningMetrics.CapacityIdleRate = float32(math.Min(float64(clusterRunningMetrics.CapacityIdleRate),
				float64(resourcePoolRunningMetrics.CapacityIdleRate)))
			clusterRunningMetrics.CapacityIdleRateAbsolute = float32(math.Min(float64(clusterRunningMetrics.CapacityIdleRateAbsolute),
				float64(resourcePoolRunningMetrics.CapacityIdleRateAbsolute)))
			valideDcCount++
		}

		clusterRunningMetrics.Bps = clusterRunningMetrics.BpsRead + clusterRunningMetrics.BpsWrite
		clusterRunningMetrics.P99 /= float32(valideDcCount)
		clusterRunningMetrics.P999 /= float32(valideDcCount)
		if status := clusterMerticsModel.Insert(clusterRunningMetrics); !status.Ok() {
			collector.ctx.Log().Error(fmt.Sprintf("Insert cluster running metrics of [%s] failed! error: %s",
				cluster.Name, status.Error()))
		}
	}
}

func (collector *ReportDataCollector) getAllTableMetrics(timeType TimeType) {
	tableModel := NewTableModel(collector.ctx)
	tableListParams := params.TableListParams{}
	tables, _ := tableModel.List(tableListParams, false)

	wg := sync.WaitGroup{}
	wg.Add(len(tables))
	for _, table := range tables {
		tableName := table.Name
		databaseName := table.Database.Name
		partitionNumber := table.PartitionNumber
		go collector.getTableMetrics(databaseName, tableName, partitionNumber, timeType, &wg)
	}
	wg.Wait()
}

func (collector *ReportDataCollector) getTableMetrics(databaseName string, tableName string,
	partitionNumber uint32, timeType TimeType, wg *sync.WaitGroup) {
	tableMetricsModel := NewTableMetricsModel(collector.ctx)
	metrics := params.TableMetricsInfo{
		DatabaseName:    databaseName,
		TableName:       tableName,
		PartitionNumber: int32(partitionNumber),
		CollectTime:     time.Now().Unix(),
		TimeType:        int32(timeType),
	}

	metrics.KpsRead, metrics.KpsWrite, metrics.Kps, _ = collector.getTableKps(databaseName, tableName)
	metrics.BpsRead, metrics.BpsWrite, metrics.Bps, _ = collector.getTableBps(databaseName, tableName)
	metrics.DataSize, _ = collector.getTableDataSize(databaseName, tableName)
	if status := tableMetricsModel.Insert(metrics); !status.Ok() {
		collector.ctx.Log().Error(fmt.Sprintf("Insert table running metrics of [%s:%s] failed! error: %s",
			databaseName, tableName, status.Error()))
	}
	wg.Done()
}

func (collector *ReportDataCollector) getNodeCpuUsageRate(nodeIP string) (cpuUsageRate float32,
	cpuUsageRateSystem float32, cpuUsageRateUser float32, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ClientPrometheusAddress()
	// cpu usage rate
	promql := fmt.Sprintf(PromqlPatternNodeCpuUsage, nodeIP)
	if cpuUsageRate, status = collector.getMetricsWithFloat32Value(prometheusHost, promql); !status.Ok() {
		return
	}

	// cpu usage rate system
	promql = fmt.Sprintf(PromqlPatternNodeCpuSystem, nodeIP)
	if cpuUsageRateSystem, status = collector.getMetricsWithFloat32Value(prometheusHost, promql); !status.Ok() {
		return
	}

	// cpu usage rate user
	promql = fmt.Sprintf(PromqlPatternNodeCpuUser, nodeIP)
	if cpuUsageRateUser, status = collector.getMetricsWithFloat32Value(prometheusHost, promql); !status.Ok() {
		return
	}
	return
}

func (collector *ReportDataCollector) getNodeDiskMetrics(nodeIP string) (diskSizeTotal int64,
	diskSizeUsage int64, diskUsageRate float32, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ClientPrometheusAddress()
	// disk total size
	promql := fmt.Sprintf(PromqlPatternNodeDiskSizeTotal, nodeIP)
	diskSizeTotal, metrics, status := collector.getMetricsWithInt64Value(prometheusHost, promql)
	if !status.Ok() {
		return
	}

	mountPoint, ok := metrics["mountpoint"]
	if !ok {
		status = common.StatusError(common.ReportPrometheusInvalidResponse)
		return
	}

	// disk usage size
	promql = fmt.Sprintf(PromqlPatternNodeDiskSizeFree, nodeIP, mountPoint)
	diskSizeFree, _, status := collector.getMetricsWithInt64Value(prometheusHost, promql)
	if !status.Ok() {
		return
	}
	diskSizeUsage = diskSizeTotal - diskSizeFree

	// disk usage rate
	diskUsageRate = float32(float64(diskSizeUsage) / float64(diskSizeTotal) * 100)
	return
}

func (collector *ReportDataCollector) getNodeMemoryMetrics(nodeIP string) (memSizeTotal int64,
	memSizeUsage int64, memUsageRate float32, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ClientPrometheusAddress()
	// mem total size
	promql := fmt.Sprintf(PromqlPatternNodeMemSizeTotal, nodeIP)
	if memSizeTotal, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql); !status.Ok() {
		return
	}

	// mem available size
	promql = fmt.Sprintf(PromqlPatternNodeMemSizeAvailable, nodeIP)
	var memSizeAvailable int64
	if memSizeAvailable, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql); !status.Ok() {
		return
	}
	memSizeUsage = memSizeTotal - memSizeAvailable

	// mem usage rate
	memUsageRate = float32(float64(memSizeUsage) / float64(memSizeTotal) * 100)
	return
}

func (collector *ReportDataCollector) getNodeNetworkMetrics(nodeIP string) (netIn int64,
	netOut int64, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ClientPrometheusAddress()
	// net in
	promql := fmt.Sprintf(PromqlPatternNodeNetIn, nodeIP)
	if netIn, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql); !status.Ok() {
		return
	}

	// net out
	promql = fmt.Sprintf(PromqlPatternNodeNetOut, nodeIP)
	netOut, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql)
	return
}

func (collector *ReportDataCollector) getNodeIoUsageRate(nodeIP string) (ioUsageRate float32,
	status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ClientPrometheusAddress()
	promql := fmt.Sprintf(PromqlPatternNodeIOUsageRate, nodeIP)
	ioUsageRate, status = collector.getMetricsWithFloat32Value(prometheusHost, promql)
	return
}

func (collector *ReportDataCollector) getNodeQps(nodeIP string, port uint16) (qps int32, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ServerPrometheusAddress()
	promql := fmt.Sprintf(PromqlPatternNodeQps, nodeIP, port)
	qps, _, status = collector.getMetricsWithInt32Value(prometheusHost, promql)
	return
}

func (collector *ReportDataCollector) getNodeKps(nodeIP string, port uint16) (kpsRead int32,
	kpsWrite int32, kpsTotal int32, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ServerPrometheusAddress()
	promql := fmt.Sprintf(PromqlPatternNodeReadKps, nodeIP, port)
	if kpsRead, _, status = collector.getMetricsWithInt32Value(prometheusHost, promql); !status.Ok() {
		return
	}

	promql = fmt.Sprintf(PromqlPatternNodeWriteKps, nodeIP, port)
	if kpsWrite, _, status = collector.getMetricsWithInt32Value(prometheusHost, promql); !status.Ok() {
		return
	}
	kpsTotal = kpsRead + kpsWrite
	return
}

func (collector *ReportDataCollector) getNodeBps(nodeIP string, port uint16) (bpsRead int64,
	bpsWrite int64, bpsTotal int64, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ServerPrometheusAddress()
	promql := fmt.Sprintf(PromqlPatternNodeReadBps, nodeIP, port)
	if bpsRead, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql); !status.Ok() {
		return
	}

	promql = fmt.Sprintf(PromqlPatternNodeWriteBps, nodeIP, port)
	if bpsWrite, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql); !status.Ok() {
		return
	}
	bpsTotal = bpsRead + bpsWrite
	return
}

func (collector *ReportDataCollector) getNodeDelayTime(nodeIP string, port uint16) (p99 float32,
	p999 float32, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ServerPrometheusAddress()
	promql := fmt.Sprintf(PromqlPatternNodeP99, nodeIP, port)
	if p99, status = collector.getMetricsWithFloat32Value(prometheusHost, promql); !status.Ok() {
		return
	}

	promql = fmt.Sprintf(PromqlPatternNodeP999, nodeIP, port)
	p999, status = collector.getMetricsWithFloat32Value(prometheusHost, promql)
	return
}

func (collector *ReportDataCollector) getNodeDataSize(nodeIP string, port uint16) (dataSize int64,
	status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ServerPrometheusAddress()
	promql := fmt.Sprintf(PromqlPatternNodeDataSize, nodeIP, port)
	dataSize, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql)
	return
}

func (collector *ReportDataCollector) getJitterDuration(nodeIP string, port uint16) (duration int32,
	status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ServerPrometheusAddress()
	instance := fmt.Sprintf("%s:%d", nodeIP, port)
	promql := fmt.Sprintf(PromqlPatternNodeJitterDuration, instance, instance)
	duration, _, _ = collector.getMetricsWithInt32Value(prometheusHost, promql)
	return
}

func (collector *ReportDataCollector) getTableDataSize(databaseName string, tableName string) (dataSize int64,
	status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ServerPrometheusAddress()
	serviceName := collector.getServiceName()
	promql := fmt.Sprintf(PromqlPatternTableDataSize, serviceName, databaseName, tableName)
	dataSize, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql)
	return
}

func (collector *ReportDataCollector) getTableKps(databaseName string, tableName string) (kpsRead int32,
	kpsWrite int32, kpsTotal int32, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ServerPrometheusAddress()
	serviceName := collector.getServiceName()
	promql := fmt.Sprintf(PromqlPatternTableReadKps, serviceName, databaseName, tableName)
	if kpsRead, _, status = collector.getMetricsWithInt32Value(prometheusHost, promql); !status.Ok() {
		return
	}

	promql = fmt.Sprintf(PromqlPatternTableWriteKps, serviceName, databaseName, tableName)
	if kpsWrite, _, status = collector.getMetricsWithInt32Value(prometheusHost, promql); !status.Ok() {
		return
	}
	kpsTotal = kpsRead + kpsWrite
	return
}

func (collector *ReportDataCollector) getTableBps(databaseName string, tableName string) (bpsRead int64,
	bpsWrite int64, bpsTotal int64, status *common.Status) {
	prometheusHost := collector.ctx.GetConfig().ServerPrometheusAddress()
	serviceName := collector.getServiceName()
	promql := fmt.Sprintf(PromqlPatternTableReadBps, serviceName, databaseName, tableName)
	if bpsRead, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql); !status.Ok() {
		return
	}

	promql = fmt.Sprintf(PromqlPatternTableWriteBps, serviceName, databaseName, tableName)
	if bpsWrite, _, status = collector.getMetricsWithInt64Value(prometheusHost, promql); !status.Ok() {
		return
	}
	bpsTotal = bpsRead + bpsWrite
	return
}

func (collector *ReportDataCollector) prometheusQuery(host string, promql string) (PrometheusResponse, *common.Status) {
	url := "http://" + host + "/api/v1/query?query=" + url.PathEscape(promql)
	var prometheusResponse PrometheusResponse
	resp, err := http.Get(url)
	if err != nil {
		collector.ctx.Log().Error(fmt.Sprintf("Prometheus http get failed. promql: %s, error: %s", promql, err.Error()))
		return prometheusResponse, common.StatusWithError(err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		collector.ctx.Log().Error(fmt.Sprintf("Prometheus http get status error. promql: %s, status: %s",
			promql, resp.Status))
		return prometheusResponse, common.StatusWithMessage(common.HttpRequestError, resp.Status)
	}

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		collector.ctx.Log().Error(fmt.Sprintf("Prometheus read response body error. promql: %s, error: %s",
			promql, err.Error()))
		return prometheusResponse, common.StatusWithError(err)
	}

	err = json.Unmarshal(body, &prometheusResponse)
	if err != nil {
		collector.ctx.Log().Error(fmt.Sprintf("Prometheus response unmarshal error. promql: %s, body:%s,  error: %s",
			promql, body, err.Error()))
		return prometheusResponse, common.StatusWithError(err)
	}

	return prometheusResponse, common.StatusOk()
}

func (collector *ReportDataCollector) getSingleValueMetrics(prometheusHost string, promql string) (metricValue string,
	metrics map[string]string, status *common.Status) {
	prometheusResponse, status := collector.prometheusQuery(prometheusHost, promql)
	if !status.Ok() {
		return
	}

	if prometheusResponse.Status != "success" {
		collector.ctx.Log().Error(fmt.Sprintf("Prometheus response status error. promql: %s, status: %s",
			promql, prometheusResponse.Status))
		status = common.StatusError(common.ReportQueryPrometheusFailed)
		return
	}

	if len(prometheusResponse.Data.Result) < 1 {
		collector.ctx.Log().Debug(fmt.Sprintf("Prometheus response result size < 1. promql: %s", promql))
		status = common.StatusError(common.ReportPrometheusEmptyResult)
		return
	}

	if len(prometheusResponse.Data.Result[0].Value) < 2 {
		collector.ctx.Log().Error(fmt.Sprintf("Prometheus response result value size < 2. promql: %s", promql))
		status = common.StatusError(common.ReportPrometheusInvalidValueStruct)
		return
	}

	metricValue, ok := prometheusResponse.Data.Result[0].Value[1].(string)
	if !ok {
		collector.ctx.Log().Error(fmt.Sprintf("Prometheus response result value type is not string. promql: %s", promql))
		status = common.StatusError(common.ReportPrometheusInvalidValueType)
		return
	}
	metrics = prometheusResponse.Data.Result[0].Metric
	return
}

func (collector *ReportDataCollector) getMetricsWithFloat64Value(prometheusHost string,
	promql string) (metricValue float64, metrics map[string]string, status *common.Status) {
	metricValueStr, metrics, status := collector.getSingleValueMetrics(prometheusHost, promql)
	if !status.Ok() {
		return
	}
	metricValue, err := strconv.ParseFloat(metricValueStr, 64)
	if err != nil {
		collector.ctx.Log().Error(fmt.Sprintf("Prometheus metric value parse to float failed. promql: %s, value: %s",
			promql, metricValueStr))
		status = common.StatusWithError(err)
		return
	}
	return
}

func (collector *ReportDataCollector) getMetricsWithInt64Value(prometheusHost string,
	promql string) (metricValue int64, metrics map[string]string, status *common.Status) {
	metricValueFloat64, metrics, status := collector.getMetricsWithFloat64Value(prometheusHost, promql)
	metricValue = int64(metricValueFloat64)
	return
}

func (collector *ReportDataCollector) getMetricsWithInt32Value(prometheusHost string,
	promql string) (metricValue int32, metrics map[string]string, status *common.Status) {
	metricValueFloat64, metrics, status := collector.getMetricsWithFloat64Value(prometheusHost, promql)
	metricValue = int32(metricValueFloat64)
	return
}

func (collector *ReportDataCollector) getMetricsWithFloat32Value(prometheusHost string,
	promql string) (metricValue float32, status *common.Status) {
	metricValueFloat64, _, status := collector.getMetricsWithFloat64Value(prometheusHost, promql)
	metricValue = float32(metricValueFloat64)
	return
}

func (collector *ReportDataCollector) getServiceName() string {
	serviceName := "laser_" + collector.ctx.GetConfig().ServiceName()
	return serviceName
}
