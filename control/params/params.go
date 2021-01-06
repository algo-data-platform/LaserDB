package params

type KeyCheckParams struct {
	Database string   `json:"dbName"   binding:"required"`
	Table    string   `json:"tbName"   binding:"required"`
	Key      []string `json:"keyName"  binding:"required"`
	Checkall bool     `json:"Checkall" binding:"-"`
}

type KeyCheckIdInfo struct {
	PartitionId string `json:"PartitionId"`
	ShardId     string `json:"ShardId"`
}

type KeyCheckDto struct {
	Role   string `json:"Role"`
	Ip     string `json:"Ip"`
	Port   uint16 `json:"Port"`
	Dc     string `json:"Dc"`
	Status string `json:"Status"`
	Value  string `json:"Value"`
}

type DatabaseAddParams struct {
	Name string `json:"Name" binding:"required"`
	Desc string `json:"Desc" binding:"-"`
}

type DatabaseListParams struct {
	Page  uint32 `json:"Page" binding:"-"`
	Limit uint32 `json:"Limit" binding:"-"`
}

type DatabaseDto struct {
	Id   uint32 `json:"Id"`
	Name string `json:"Name"`
	Desc string `json:"Desc"`
}

type DatabaseUpdateParams struct {
	Id   uint32 `json:"Id"`
	Name string `json:"Name"`
	Desc string `json:"desc"`
}

type DatabaseDeleteParams struct {
	Id uint32 `json:"DatabaseId" binding:"required"`
}

type LaserCommandParams struct {
	Name          string `json:"Name" binding:"required"`
	OperationType uint32 `json:"OperationType" bind:"required"`
}

type TrafficRestrictionLimitParams struct {
	Id            uint32  `json:"Id" binding:"gte=0"`
	Name          string  `json:"Name" binding:"required"`
	OperationType uint32  `json:"OperationType" binding:"required"`
	LimitType     *uint32 `json:"LimitType" binding:"required"`
	LimitValue    uint32  `json:"LimitValue" binding:"required"`
}

type TableAddParams struct {
	Name            string                          `json:"Name" binding:"required"`
	Status          *uint32                         `json:"Status" binding:"oneof=0 1"`
	DenyAll         *uint32                         `json:"DenyAll" binding:"oneof=0 1"`
	PartitionNumber uint32                          `json:"PartitionNumber" binding:"required"`
	Ttl             *uint64                         `json:"Ttl" binding:"-"`
	Desc            string                          `json:"Desc" binding:"-"`
	DcId            uint32                          `json:"DcId" binding:"gte=0"`
	DistDcId        uint32                          `json:"DistDcId" binding:"gte=0"`
	DatabaseId      uint32                          `json:"DatabaseId" binding:"gte=0"`
	ConfigId        uint32                          `json:"ConfigId" binding:"gte=0"`
	EdgeFlowRatio   *uint32                         `json:"EdgeFlowRatio" binding:"-"`
	TrafficLimits   []TrafficRestrictionLimitParams `json:"TrafficLimits" binding:"-"`
	BindEdgeNodeIds []uint32                        `json:"BindEdgeNodeIds" binding:"-"`
}

type TableListParams struct {
	Page         uint32 `json:"Page" binding:"-"`
	Limit        uint32 `json:"Limit" binding:"-"`
	DatabaseId   uint32 `json:"DatabaseId" binding:"-"`
	TableId      uint32 `json:"TableId" binding:"-"`
	DatabaseName string `json:"DatabaseName" binding:"-"`
	TableName    string `json:"TableName" binding:"-"`
}

type TableDto struct {
	Id              uint32                          `json:"Id"`
	Name            string                          `json:"Name"`
	Status          uint32                          `json:"Status"`
	DenyAll         uint32                          `json:"DenyAll"`
	PartitionNumber uint32                          `json:"PartitionNumber"`
	Ttl             uint64                          `json:"Ttl"`
	Desc            string                          `json:"Desc"`
	DcId            uint32                          `json:"DcId"`
	DcName          string                          `json:"DcName"`
	DistDcId        uint32                          `json:"DistDcId"`
	DistDcName      string                          `json:"DistDcName"`
	DatabaseId      uint32                          `json:"DatabaseId"`
	ConfigId        uint32                          `json:"ConfigId"`
	DatabaseName    string                          `json:"DatabaseName"`
	ConfigName      string                          `json:"ConfigName"`
	EdgeFlowRatio   *uint32                         `json:"EdgeFlowRatio" binding:"-"`
	TrafficLimits   []TrafficRestrictionLimitParams `json:"TrafficLimits"`
	BindEdgeNodes   []NodeInfo                      `json:"BindEdgeNodes"`
}

type TableUpdateParams struct {
	Id              uint32                          `json:"Id" binding:"gte=0"`
	Name            string                          `json:"Name" binding:"required"`
	Status          *uint32                         `json:"Status" binding:"oneof=0 1"`
	DenyAll         *uint32                         `json:"DenyAll" binding:"oneof=0 1"`
	Ttl             *uint64                         `json:"Ttl" binding:"gte=0"`
	Desc            string                          `json:"Desc" binding:"-"`
	DcId            uint32                          `json:"DcId" binding:"gte=0"`
	DistDcId        uint32                          `json:"DistDcId" binding:"gte=0"`
	DatabaseId      uint32                          `json:"DatabaseId" binding:"gte=0"`
	ConfigId        uint32                          `json:"ConfigId" binding:"gte=0"`
	BindEdgeNodes   *string                         `json:"BindEdgeNodes" binding:"-"`
	EdgeFlowRatio   *uint32                         `json:"EdgeFlowRatio" binding:"-"`
	TrafficLimits   []TrafficRestrictionLimitParams `json:"TrafficLimits" binding:"-"`
	BindEdgeNodeIds []uint32                        `json:"BindEdgeNodeIds" binding:"-"`
}

type TableDeleteParams struct {
	Id uint32 `json:"TableId" binding:"required"`
}

type TableConfigItemParams struct {
	Id    uint32 `json:"Id" binding:"gte=0"`
	Name  string `json:"Name" binding:"required"`
	Value string `json:"Value" binding:"required"`
	Type  uint32 `json:"Type" binding:"required"`
}

type TableConfigListParams struct {
	Page          uint32   `json:"Page" binding:"-"`
	Limit         uint32   `json:"Limit" binding:"-"`
	Name          string   `json:"Name" binding:"-"`
	ExcludedNames []string `form:"ExcludedNames[]" json:"ExcludedNames[]" binding:"-"`
}

type TableConfigParams struct {
	Id          uint32                  `json:"Id" binding:"gte=0"`
	Name        string                  `json:"Name" binding:"required"`
	Version     uint32                  `json:"Version" binding:"gte=0"`
	IsDefault   *uint32                 `json:"IsDefault" binding:"required"`
	Desc        string                  `json:"Desc" binding:"-"`
	ConfigItems []TableConfigItemParams `json:"ConfigItems" binding:"-"`
}

type TableConfigDeleteParams struct {
	Id uint32 `json:"TableConfigId" binding:"required"`
}

type ProxyTableConfigParams struct {
	Id           uint32 `json:"Id" binding:"gte=0"`
	DatabaseName string `json:"DatabaseName" binding:"required"`
	TableName    string `json:"TableName" binding:"required"`
	ReadTimeout  uint32 `json:"ReadTimeout" binding:"required"`
	WriteTimeout uint32 `json:"WriteTimeout" binding:"required"`
	AllowedFlow  uint32 `json:"AllowedFlow" binding:"required"`
}

type ProxyTableConfigDto struct {
	Id           uint32 `json:"Id"`
	DatabaseName string `json:"DatabaseName"`
	TableName    string `json:"TableName"`
	ReadTimeout  uint32 `json:"ReadTimeout"`
	WriteTimeout uint32 `json:"WriteTimeout"`
	AllowedFlow  uint32 `json:"AllowedFlow"`
}

type ProxyTableConfigListParams struct {
	Page         uint32 `json:"Page" binding:"-"`
	Limit        uint32 `json:"Limit" binding:"-"`
	DatabaseName string `json:"DatabaseName" binding:"-"`
}

type NodeRateLimitEntryParams struct {
	Id              uint32  `json:"Id" binding:"gte=0"`
	BeginHour       *uint32 `json:"BeginHour" binding:"required"`
	EndHour         uint32  `json:"EndHour" binding:"required"`
	RateBytesPerSec int64   `json:"RateBytesPerSec" binding:"required"`
}

type NodeConfigParams struct {
	Id                  uint32                     `json:"Id" binding:"gte=0"`
	Name                string                     `json:"Name" binding:"required"`
	Desc                string                     `json:"Desc" binding:"-"`
	BlockCacheSizeGb    uint32                     `json:"BlockCacheSizeGb" binding:"required"`
	WriteBufferSizeGb   uint32                     `json:"WriteBufferSizeGb" binding:"required"`
	NumShardBits        int32                      `json:"NumShardBits" binding:"required"`
	HighPriPoolRatio    *float64                   `json:"HighPriPoolRatio" binding:"required"`
	StrictCapacityLimit uint32                     `json:"StrictCapacityLimit" binding:"-"`
	RateLimitStrategy   []NodeRateLimitEntryParams `json:"RateLimitStrategy" binding:"-"`
}

type NodeConfigDto struct {
	Id                  uint32                     `json:"Id"`
	Name                string                     `json:"Name"`
	Desc                string                     `json:"Desc"`
	BlockCacheSizeGb    uint32                     `json:"BlockCacheSizeGb"`
	WriteBufferSizeGb   uint32                     `json:"WriteBufferSizeGb"`
	NumShardBits        int32                      `json:"NumShardBits"`
	HighPriPoolRatio    *float64                   `json:"HighPriPoolRatio"`
	StrictCapacityLimit uint32                     `json:"StrictCapacityLimit"`
	RateLimitStrategy   []NodeRateLimitEntryParams `json:"RateLimitStrategy" binding:"-"`
}

type NodeConfigListParams struct {
	Page  uint32 `json:"Page" binding:"-"`
	Limit uint32 `json:"Limit" binding:"-"`
	Name  string `json:"Name" binding:"-"`
}

type NodeConfigDeleteParams struct {
	Id uint32 `json:"NodeConfigId" binding:"required"`
}

type UserInfoDto struct {
	Uid     string `json:"Uid"`
	Name    string `json:"Name"`
	Email   string `json:"Email"`
	IsAdmin bool   `json:"IsAdmin"`
}

type RouterConfig struct {
	TotalShards  uint32 `json:"TotalShards"`
	TtlInMs      uint32 `json:"TtlInMs"`
	PullInterval uint32 `json:"PullInterval"`
	LoadBalance  string `json:"LoadBalance"`
}

type ServiceAdd struct {
	ServiceName  string       `json:"ServiceName"`
	RouterConfig RouterConfig `json:"RouterConfig"`
}

type ServiceServer struct {
	Host                       string            `json:"Host"`
	Port                       uint16            `json:"Port"`
	ServiceName                string            `json:"ServiceName"`
	Protocol                   string            `json:"Protocol"`
	Status                     string            `json:"Status"`
	UpdateTime                 string            `json:"UpdateTime"`
	Weight                     string            `json:"Weight"`
	ShardList                  []uint32          `json:"ShardList"`
	AvailableShardList         []uint32          `json:"AvailableShardList"`
	FollowerShardList          []uint32          `json:"FollowerShardList"`
	FollowerAvailableShardList []uint32          `json:"FollowerAvailableShardList"`
	OtherSettings              map[string]string `json:"OtherSettings"`
}

type NodeInfoAttr struct {
	Name  string `json:"Name"`
	Value string `json:"Value"`
}

type DcParams struct {
	Id          uint32 `json:"Id" binding:"-"`
	ClusterId   uint32 `json:"ClusterId" binding:"required"`
	ClusterName string `json:"ClusterName" binding:"required"`
	Name        string `json:"Name" binding:"required"`
	ShardNumber uint32 `json:"ShardNumber" binding:"required"`
	Desc        string `json:"Desc" binding:"-"`
}

type DcListParams struct {
	Page  uint32 `json:"Page" binding:"-"`
	Limit uint32 `json:"Limit" binding:"-"`
}

type ClusterParams struct {
	Id         uint32 `json:"Id" binding:"-"`
	Name       string `json:"Name" binding:"required"`
	Alias      string `json:"Alias" binding:"required"`
	ShardTotal uint32 `json:"ShardTotal" binding:"required"`
	Desc       string `json:"Desc" binding:"-"`
}

type ClusterListParams struct {
	Page  uint32 `json:"Page" binding:"-"`
	Limit uint32 `json:"Limit" binding:"-"`
}

type GroupParams struct {
	Id           uint32 `json:"Id" binding:"-"`
	Name         string `json:"Name" binding:"required"`
	Alias        string `json:"Alias" binding:"required"`
	NodeConfigId uint32 `json:"NodeConfigId" binding:"-"`
	Desc         string `json:"Desc" binding:"-"`
	DcId         uint32 `json:"DcId" binding:"required"`
	DcName       string `json:"DcName" binding:"-"`
	ClusterId    uint32 `json:"ClusterId" binding:"required"`
	ClusterName  string `json:"ClusterName" binding:"required"`
}

type GroupListParams struct {
	Page        uint32 `json:"Page" binding:"-"`
	Limit       uint32 `json:"Limit" binding:"-"`
	ClusterId   uint32 `json:"ClusterId" binding:"-"`
	GroupId     uint32 `json:"GroupId" binding:"-"`
	GroupName   string `json:"GroupName" binding:"-"`
	ShowDetails bool   `json:"ShowDetails" binding:"-"`
}

type GroupDeleteParams struct {
	Id uint32 `json:"GroupId" binding:"required"`
}

type GroupDto struct {
	Id             uint32 `json:"Id" binding:"-"`
	Name           string `json:"Name" binding:"required"`
	Alias          string `json:"Alias" binding:"required"`
	NodeConfigId   uint32 `json:"NodeConfigId" binding:"-"`
	NodeConfigName string `json:"NodeConfigName" binding:"-"`
	ActiveNumber   uint32 `json:"ActiveNumber" binding:"-"`
	InactiveNumber uint32 `json:"InactiveNumber" binding:"-"`
	Desc           string `json:"Desc" binding:"-"`
	DcId           uint32 `json:"DcId" binding:"required"`
	DcName         string `json:"DcName" binding:"required"`
	ClusterId      uint32 `json:"ClusterId" binding:"required"`
	ClusterName    string `json:"ClusterName" binding:"required"`
}

type NodeInfo struct {
	Id                   uint32         `json:"Id"`
	NodeId               uint32         `json:"NodeId"`
	Active               *uint16        `json:"Active"`
	Status               *uint16        `json:"Status"`
	Host                 string         `json:"Host"`
	Port                 uint16         `json:"Port"`
	Weight               *uint32        `json:"Weight"`
	ShardNumber          uint16         `json:"ShardNumber"`
	AvailableShardNumber uint16         `json:"AvailableShardNumber"`
	Master               bool           `json:"Master"`
	IsEdgeNode           bool           `json:"IsEdgeNode"`
	Attrs                []NodeInfoAttr `json:"Attrs"`
	Desc                 string         `json:"Desc"`
	GroupId              uint32         `json:"GroupId"`
	GroupName            string         `json:"GroupName"`
	Dc                   string         `json:"Dc"`
	ConfigId             uint32         `json:"ConfigId"`
	ConfigName           string         `json:"ConfigName"`
	AnsibleConfigId      uint32         `json:"AnsibleConfigId"`
	AnsibleConfigName    string         `json:"AnsibleConfigName"`
	LeaderShardList      *string        `json:"LeaderShardList"`
	FollowerShardList    *string        `json:"FollowerShardList"`
	// 动态指标
	IsAvailable bool   `json:"IsAvailable"`
	ReadKps     uint64 `json:"ReadKps"`
	WriteKps    uint64 `json:"WriteKps"`
}

type NodeGroup struct {
	Id        uint32     `json:"Id"`
	GroupName string     `json:"GroupName"`
	Dc        string     `json:"Dc"`
	DescName  string     `json:"DescName"`
	Nodes     []NodeInfo `json:"Nodes"`
}

type NodeParams struct {
	Id                uint32  `json:"Id" binding:"-"`
	NodeId            uint32  `json:"NodeId" binding:"required"`
	Active            *uint16 `json:"Active" binding:"required"`
	Status            *uint16 `json:"Status" binding:"required"`
	Host              string  `json:"Host" binding:"required"`
	Port              uint16  `json:"Port" binding:"required"`
	Weight            *uint32 `json:"Weight" binding:"required"`
	Master            *bool   `json:"Master" binding:"required"`
	IsEdgeNode        *bool   `json:"IsEdgeNode" binding:"required"`
	Desc              string  `json:"Desc" binding:"-"`
	GroupId           uint32  `json:"GroupId" binding:"required"`
	GroupName         string  `json:"GroupName" binding:"-"`
	ConfigId          uint32  `json:"ConfigId" binding:"required"`
	ConfigName        string  `json:"ConfigName" binding:"-"`
	AnsibleConfigId   uint32  `json:"AnsibleConfigId" binding:"required"`
	AnsibleConfigName string  `json:"AnsibleConfigName" binding:"-"`
	LeaderShardList   *string `json:"LeaderShardList" binding:"-"`
	FollowerShardList *string `json:"FollowerShardList" binding:"-"`
}

type NodeBatchStoreParams struct {
	Nodes []NodeParams `json:"Nodes" binding:"required"`
}

type NodeListParams struct {
	Page       uint32 `json:"Page" bindding:"-"`
	Limit      uint32 `json:"Limit" binding:"-"`
	GroupId    uint32 `json:"GroupId" binding:"-"`
	IsEdgeNode *bool  `json:"IsEdgeNode" binding:"-"`
}

type NodeDeleteParams struct {
	Id uint32 `json:"Id" binding:"required"`
}

type NodeBatchDeleteParams struct {
	Ids []uint32 `json:"Ids" binding:"required"`
}

type NodeChangeRoleParams struct {
	Id      uint32 `json:"Id" binding:"-"`
	GroupId uint32 `json:"GroupId" binding:"required"`
	Master  *bool  `json:"Master" binding:"required"`
}

type MetricsNodeAddress struct {
	ServiceAddress string `json:"ServiceAddress"`
}

type NodeAddressListParams struct {
	StartTime int64 `json:"StartTime" binding:"-"`
	EndTime   int64 `json:"EndTime" binding:"-"`
	TimeType  int32 `json:"TimeType" binding:"-"`
}

type NodePhysicalMetricsInfo struct {
	ServiceAddress           string  `json:"ServiceAddress"`
	CpuUsageRate             float32 `json:"CpuUsageRate"`
	CpuUsageRateUser         float32 `json:"CpuUsageRateUser"`
	CpuUsageRateSystem       float32 `json:"CpuUsageRateSystem"`
	MemorySizeTotal          int64   `json:"MemorySizeTotal"`
	MemorySizeUsage          int64   `json:"MemorySizeUsage"`
	MemoryUsageRate          float32 `json:"MemoryUsageRate"`
	DiskSizeTotal            int64   `json:"DiskSizeTotal"`
	DiskSizeUsage            int64   `json:"DiskSizeUsage"`
	DiskUsageRate            float32 `json:"DiskUsageRate"`
	IoUsageRate              float32 `json:"IoUsageRate"`
	NetworkIn                int64   `json:"NetworkIn"`
	NetworkOut               int64   `json:"NetwordOut"`
	CollectTime              int64   `json:"CollectTime"`
	TimeType                 int32   `json:"TimeType"`
	CapacityIdleRateAbsolute float32 `json:"CapacityIdleRateAbsolute"`
}

type NodePhysicalMetricsListParams struct {
	StartTime        int64    `json:"StartTime" binding:"-"`
	EndTime          int64    `json:"EndTime" binding:"-"`
	ServiceAddresses []string `form:"ServiceAddresses[]" json:"ServiceAddresses[]" binding:"-"`
	TimeType         int32    `json:"TimeType" binding:"-"`
	Page             uint32   `json:"Page" binding:"-"`
	Limit            uint32   `json:"Limit" binding:"-"`
}

type NodeRunningMetricsInfo struct {
	Id                       int32   `json:"Id"`
	ServiceAddress           string  `json:"ServiceAddress"`
	Qps                      int32   `json:"Qps"`
	Kps                      int32   `json:"Kps"`
	KpsWrite                 int32   `json:"KpsWrite"`
	KpsRead                  int32   `json:"KpsRead"`
	Bps                      int64   `json:"Bps"`
	BpsWrite                 int64   `json:"BpsWrite"`
	BpsRead                  int64   `json:"BpsRead"`
	DataSize                 int64   `json:"DataSize"`
	P99                      float32 `json:"P99"`
	P999                     float32 `json:"P999"`
	JitterDuration           int32   `json:"JitterDuration"`
	CapacityIdleRate         float32 `json:"CapacityIdleRate"`
	CapacityIdleRateAbsolute float32 `json:"CapacityIdleRateAbsolute"`
	CollectTime              int64   `json:"CollectTime"`
	TimeType                 int32   `json:"TimeType"`
}

type NodeRunningMetricsListParams struct {
	StartTime        int64    `json:"StartTime" binding:"-"`
	EndTime          int64    `json:"EndTime" binding:"-"`
	ServiceAddresses []string `form:"ServiceAddresses[]" json:"ServiceAddresses[]" binding:"-"`
	TimeType         int32    `json:"TimeType" binding:"-"`
	Page             uint32   `json:"Page" binding:"-"`
	Limit            uint32   `json:"Limit" binding:"-"`
}

type MetricsGroupName struct {
	GroupName string `json:"GroupName"`
}

type MetricGroupListParams struct {
	StartTime int64 `json:"StartTime" binding:"-"`
	EndTime   int64 `json:"EndTime" binding:"-"`
	TimeType  int32 `json:"TimeType" binding:"-"`
}

type GroupRunningMetricsInfo struct {
	Id                       int32   `json:"Id"`
	GroupName                string  `json:"GroupName"`
	Qps                      int32   `json:"Qps"`
	Kps                      int32   `json:"Kps"`
	KpsWrite                 int32   `json:"KpsWrite"`
	KpsRead                  int32   `json:"KpsRead"`
	Bps                      int64   `json:"Bps"`
	BpsWrite                 int64   `json:"BpsWrite"`
	BpsRead                  int64   `json:"BpsRead"`
	DataSize                 int64   `json:"DataSize"`
	P99                      float32 `json:"P99"`
	P999                     float32 `json:"P999"`
	JitterDuration           int32   `json:"JitterDuration"`
	CapacityIdleRate         float32 `json:"CapacityIdleRate"`
	CapacityIdleRateAbsolute float32 `json:"CapacityIdleRateAbsolute"`
	CollectTime              int64   `json:"CollectTime"`
	TimeType                 int32   `json:"TimeType"`
}

type GroupRunningMetricsListParams struct {
	StartTime  int64    `json:"StartTime" binding:"-"`
	EndTime    int64    `json:"EndTime" binding:"-"`
	GroupNames []string `form:"GroupNames[]" json:"GroupNames[]" binding:"-"`
	TimeType   int32    `json:"TimeType" binding:"-"`
	Page       uint32   `json:"Page" binding:"-"`
	Limit      uint32   `json:"Limit" binding:"-"`
}

type MetricsPoolName struct {
	PoolName string `json:"PoolName"`
}

type MetricPoolListParams struct {
	StartTime int64 `json:"StartTime" binding:"-"`
	EndTime   int64 `json:"EndTime" binding:"-"`
	TimeType  int32 `json:"TimeType" binding:"-"`
}

type ResourcePoolRunningMetricsInfo struct {
	Id                       int32   `json:"Id"`
	PoolName                 string  `json:"PoolName"`
	Qps                      int32   `json:"Qps"`
	Kps                      int32   `json:"Kps"`
	KpsWrite                 int32   `json:"KpsWrite"`
	KpsRead                  int32   `json:"KpsRead"`
	Bps                      int64   `json:"Bps"`
	BpsWrite                 int64   `json:"BpsWrite"`
	BpsRead                  int64   `json:"BpsRead"`
	DataSize                 int64   `json:"DataSize"`
	P99                      float32 `json:"P99"`
	P999                     float32 `json:"P999"`
	JitterDuration           int32   `json:"JitterDuration"`
	CapacityIdleRate         float32 `json:"CapacityIdleRate"`
	CapacityIdleRateAbsolute float32 `json:"CapacityIdleRateAbsolute"`
	CollectTime              int64   `json:"CollectTime"`
	TimeType                 int32   `json:"TimeType"`
}

type ResourcePoolRunningMetricsListParams struct {
	StartTime int64    `json:"StartTime" binding:"-"`
	EndTime   int64    `json:"EndTime" binding:"-"`
	PoolNames []string `form:"PoolNames[]" json:"PoolName[]" binding:"-"`
	TimeType  int32    `json:"TimeType" binding:"-"`
	Page      uint32   `json:"Page" binding:"-"`
	Limit     uint32   `json:"Limit" binding:"-"`
}

type ClusterRunningMetricsInfo struct {
	Id                       int32   `json:"Id"`
	Qps                      int32   `json:"Qps"`
	Kps                      int32   `json:"Kps"`
	KpsWrite                 int32   `json:"KpsWrite"`
	KpsRead                  int32   `json:"KpsRead"`
	Bps                      int64   `json:"Bps"`
	BpsWrite                 int64   `json:"BpsWrite"`
	BpsRead                  int64   `json:"BpsRead"`
	DataSize                 int64   `json:"DataSize"`
	P99                      float32 `json:"P99"`
	P999                     float32 `json:"P999"`
	JitterDuration           int32   `json:"JitterDuration"`
	CapacityIdleRate         float32 `json:"CapacityIdleRate"`
	CapacityIdleRateAbsolute float32 `json:"CapacityIdleRateAbsolute"`
	CollectTime              int64   `json:"CollectTime"`
	TimeType                 int32   `json:"TimeType"`
}

type ClusterRunningMetricsListParams struct {
	StartTime int64  `json:"StartTime" binding:"-"`
	EndTime   int64  `json:"EndTime" binding:"-"`
	TimeType  int32  `json:"TimeType" binding:"-"`
	Page      uint32 `json:"Page" binding:"-"`
	Limit     uint32 `json:"Limit" binding:"-"`
}

type MetricTable struct {
	DatabaseName string `json:"DatabaseName" binding:"-"`
	TableName    string `json:"TableName" binding:"-"`
}

type MetricTableListParams struct {
	StartTime int64 `json:"StartTime" binding:"-"`
	EndTime   int64 `json:"EndTime" binding:"-"`
	TimeType  int32 `json:"TimeType" binding:"-"`
}

type TableMetricsInfo struct {
	Id              int32  `json:"Id"`
	DatabaseName    string `json:"DatabaseName"`
	TableName       string `json:"TableName"`
	DataSize        int64  `json:"DataSize"`
	Qps             int32  `json:"Qps"`
	Kps             int32  `json:"Kps"`
	KpsWrite        int32  `json:"KpsWrite"`
	KpsRead         int32  `json:"KpsRead"`
	Bps             int64  `json:"Bps"`
	BpsWrite        int64  `json:"BpsWrite"`
	BpsRead         int64  `json:"BpsRead"`
	PartitionNumber int32  `json:"PartitionNumber"`
	CollectTime     int64  `json:"CollectTime"`
	TimeType        int32  `json:"TimeType"`
}

type TableMetricsListParams struct {
	StartTime int64         `json:"StartTime" binding:"-"`
	EndTime   int64         `json:"EndTime" binding:"-"`
	Tables    []MetricTable `form:"Tables[]" json:"Tables[]" binding:"-"`
	TimeType  int32         `json:"TimeType" binding:"-"`
	Page      uint32        `json:"Page" binding:"-"`
	Limit     uint32        `json:"Limit" binding:"-"`
}

type SystemIndexInfo struct {
	Id                     int32   `json:"Id"`
	IoRatePerMb            float32 `json:"IoRatePerMb"`
	NetAmplificationRatio  float32 `json:"NetAmplificationRatio"`
	DiskAmplificationRatio float32 `json:"DiskAmplificationRatio"`
	CollectTime            int64   `json:"CollectTime"`
	TimeType               int32   `json:"TimeType"`
}

type SystemIndexListParams struct {
	StartTime int64  `json:"StartTime" binding:"-"`
	EndTime   int64  `json:"EndTime" binding:"-"`
	TimeType  int32  `json:"TimeType" binding:"-"`
	Page      uint32 `json:"Page" binding:"-"`
	Limit     uint32 `json:"Limit" binding:"-"`
}

type VersionChangeClear struct {
	DatabaseName string `json:"DatabaseName"`
	TableName    string `json:"TableName"`
	DatabaseId   uint32 `json:"DatabaseId"`
}

type VersionChangeVersion struct {
	DatabaseName string `json:"DatabaseName"`
	TableName    string `json:"TableName"`
	DatabaseId   uint32 `json:"DatabaseId"`
}

type VersionChangeRollback struct {
	DatabaseName string `json:"DatabaseName"`
	TableName    string `json:"TableName"`
	DatabaseId   uint32 `json:"DatabaseId"`
	Version      string `json:"VersionName"`
}

type GroupReduceMerticsParams struct {
	GroupId    uint32 `json:"GroupId" binding:"required"`
	ReduceRate uint32 `json:"ReduceRate" binding:"required"`
	ReduceMode string `json:"ReduceMode" binding:"required"`
}

type GroupReduceMetricsDto struct {
	Address   string          `json:"Address" binding:"required"`
	ShardInfo []ReduceMetrics `json:"ShardInfo" binding:"required"`
}

type ReduceMetrics struct {
	ShardId   uint32  `json:"ShardId" binding:"required"`
	ReduceNum float64 `json:"ReduceNum" binding:"required"`
}

type AssignType uint32

const (
	AssignByShardSize AssignType = iota + 1
	AssignByShardKps
	AssignByShardNum
)

type AssignShardListParams struct {
	DatabaseName    string     `json:"DatabaseName" binding:"required"`
	TableName       string     `json:"TableName" binding:"required"`
	Type            AssignType `json:"Type" binding:"required"`
	AssignedListNum uint32     `json:"AssignedListNum" binding:"required"`
}

type MachineCategoryParams struct {
	Id   uint32 `json:"Id" binding:"-"`
	Name string `json:"Name" binding:"required"`
	Desc string `json:"Desc"`
}

type MachineCategoryListParams struct {
	Page  uint32 `json:"Page" binding:"-"`
	Limit uint32 `json:"Limit" binding:"-"`
}

type MachineCategoryDeleteParams struct {
	Id uint32 `json:"Id" binding:"required"`
}

type MachineCategoryDto struct {
	Id       uint32       `json:"Id"`
	Name     string       `json:"Name"`
	Desc     string       `json:"Desc"`
	Machines []MachineDto `json:"Machines"`
}

type MachineParams struct {
	Id            uint32 `json:"Id" binding:"-"`
	CategoryId    uint32 `json:"CategoryId" binding:"required"`
	Ip            string `json:"Ip" binding:"required"`
	CpuCoreNumber string `json:"CpuCoreNumber" binding:"required"`
	MemorySizeGb  string `json:"MemorySizeGb" binding:"required"`
	Desc          string `json:"Desc" binding:"-"`
}

type MachineListParams struct {
	CategoryName string `json:"CategoryName" binding:"-"`
	MachineIp    string `json:"MachineIp" binding:"-"`
	Page         uint32 `json:"Page" binding:"-"`
	Limit        uint32 `json:"Limit" binding:"-"`
}

type MachineDto struct {
	Id                  uint32 `json:"Id"`
	CategoryId          uint32 `json:"CategoryId"`
	Ip                  string `json:"Ip"`
	CpuCoreNumber       string `json:"CpuCoreNumber"`
	MemorySizeGb        string `json:"MemorySizeGb"`
	Desc                string `json:"Desc"`
	MachineCategoryName string `json:"MachineCategoryName"`
}

type MachineUpdateParams struct {
	Id            uint32 `json:"Id" binding:"gte=0"`
	CategoryId    uint32 `json:"CategoryId" binding:"required"`
	Ip            string `json:"Ip" binding:"required"`
	CpuCoreNumber string `json:"CpuCoreNumber" binding:"required"`
	MemorySizeGb  string `json:"MemorySizeGb" binding:"required"`
	Desc          string `json:"Desc" binding:"-"`
}

type MachineDeleteParams struct {
	Id uint32 `json:"Id" binding:"gte=0"`
}

type AnsibleConfigParams struct {
	Id    uint32 `json:"Id" binding:"-"`
	Name  string `json:"Name" binding:"required"`
	Vars  string `json:"Vars" binding:"required"`
	Roles string `json:"Roles" binding:"required"`
	Desc  string `json:"Desc" binding:"-"`
}

type AnsibleConfigListParams struct {
	Page  uint32 `json:"Page" bindding:"-"`
	Limit uint32 `json:"Limit" binding:"-"`
	Id    uint32 `json:"Id" binding:"-"`
}

type AnsibleConfigDeleteParams struct {
	Id uint32 `json:"Id" binding:"gte=0"`
}

type NodeOperationParams struct {
	Host      string `json:"Host" binding:"required"`
	Port      uint16 `json:"Port" binding:"required"`
	GroupName string `json:"GroupName" binding:"-"`
	Dc        string `json:"Dc" binding:"-"`
	NodeId    uint32 `json:"NodeId" binding:"-"`
}

type NodeBatchOperationParams struct {
	Nodes         []NodeOperationParams  `json:"Nodes" binding:"required"`
	AnsibleVars   map[string]interface{} `json:"AnsibleVars" binding:"-"`
	Tags          string                 `json:"Tags" binding:"required"`
	Roles         []string               `json:"Roles" binding:"-"`
	OperationName string                 `json:"OperationName" binding:"required"`
}

type NodeBatchOperationResult struct {
	TaskId string `json:"TaskId"`
}

type NodeBatchOperationInfoParams struct {
	GroupName string `json:"GroupName"`
}

type NodeBatchOperationInfo struct {
	HasTask       bool   `json:"HasTask"`
	OperationName string `json:"OperationName"`
	Log           string `json:"Log"`
	Done          bool   `json:"Done"`
}

type TicketListParams struct {
	Loader string `json:"Loader" binding:"-"`
	Page   uint32 `json:"Page" binding:"-"`
	Limit  uint32 `json:"Limit" binding:"-"`
}

type TicketDeleteParams struct {
	Id uint32 `json:"TicketId" binding:"required"`
}

type TicketDto struct {
	Id                       uint32   `json:"Id"`
	Creator                  string   `json:"Creator"`
	SDKType                  []string `json:"SDKType"`
	BusinessLine             string   `json:"BusinessLine"`
	BusinessDescription      string   `json:"BusinessDescription"`
	BusinessKR               string   `json:"BusinessKR"`
	ReadContactPersion       string   `json:"ReadContactPersion"`
	WriteContactPersion      string   `json:"WriteContactPersion"`
	ImportDataContactPersion string   `json:"ImportDataContactPersion"`
	DatabaseName             string   `json:"DatabaseName"`
	TableName                string   `json:"TableName"`
	ReadQPS                  uint32   `json:"ReadQPS"`
	WriteQPS                 uint32   `json:"WriteQPS"`
	RequestDelayLimit        uint32   `json:"RequestDelayLimit"`
	DataExpirationTime       uint64   `json:"DataExpirationTime"`
	DataSize                 uint32   `json:"DataSize"`
	ValueSize                uint32   `json:"ValueSize"`
	CrashInfluence           string   `json:"CrashInfluence"`
	Command                  []string `json:"Command"`
	KeyNum                   uint32   `json:"KeyNum"`
	DockingPersonnel         string   `json:"DockingPersonnel"`
	PartitionNum             *uint32  `json:"PartitionNum"`
	Status                   *uint32  `json:"Status" binding:"gte=0"`
}

type TicketParams struct {
	Id                       uint32   `json:"Id" binding:"-"`
	Creator                  string   `json:"Creator" binding:"required"`
	SDKType                  []string `json:"SDKType" binding:"required"`
	BusinessLine             string   `json:"BusinessLine" binding:"-"`
	BusinessDescription      string   `json:"BusinessDescription" binding:"required"`
	BusinessKR               string   `json:"BusinessKR" binding:"-"`
	ReadContactPersion       string   `json:"ReadContactPersion" binding:"-"`
	WriteContactPersion      string   `json:"WriteContactPersion" binding:"-"`
	ImportDataContactPersion string   `json:"ImportDataContactPersion" binding:"-"`
	DatabaseName             string   `json:"DatabaseName" binding:"required"`
	TableName                string   `json:"TableName" binding:"required"`
	ReadQPS                  uint32   `json:"ReadQPS" binding:"gte=0"`
	WriteQPS                 uint32   `json:"WriteQPS" binding:"gte=0"`
	RequestDelayLimit        uint32   `json:"RequestDelayLimit" binding:"gte=0"`
	DataExpirationTime       uint64   `json:"DataExpirationTime" binding:"gte=0"`
	DataSize                 uint32   `json:"DataSize" binding:"gte=0"`
	ValueSize                uint32   `json:"ValueSize" binding:"gte=0"`
	CrashInfluence           string   `json:"CrashInfluence" binding:"-"`
	Command                  []string `json:"Command" binding:"required"`
	KeyNum                   uint32   `json:"KeyNum" binding:"-"`
	DockingPersonnel         string   `json:"DockingPersonnel" binding:"-"`
	PartitionNum             *uint32  `json:"PartitionNum" binding:"-"`
	Status                   *uint32  `json:"Status" binding:"required"`
}

type TicketProcessParams struct {
	Handler            string  `json:"Handler" binding:"required"`
	TicketAcceptStatus *uint32 `json:"TicketAcceptStatus" binding:"oneof=0 1"`
	DatabaseName       string  `json:"DatabaseName" binding:"required"`
	TableName          string  `json:"TableName" binding:"required"`
	DcId               uint32  `json:"DcId" binding:"gte=0"`
	DataExpirationTime *uint64 `json:"DataExpirationTime" binding:"required"`
	TableConfigId      *uint32 `json:"TableConfigId" binding:"required"`
	TableConfigName    string  `json:"TableConfigName" binding:"required"`
	PartitionNum       *uint32 `json:"PartitionNum" binding:"required"`
	RejectReason       string  `json:"RejectReason" binding:"-"`
}

type CheckGroupReadyToBeMasterParams struct {
	GroupName string `json:"GroupName" binding:"required"`
}

type CheckGroupReadyToBeMasterResult struct {
	Ready               bool                `json:"Ready" binding:"required"`
	MssingShards        []uint32            `json:"MissingShards"`
	ReduplicativeShards []uint32            `json:"ReduplicativeShards"`
	UnavailableShards   map[uint32][]uint32 `json:"UnavailableShards"` // nodeId: []shardId
	InconsistentNodes   []uint32            `json:"InconsistentNodes"` // 本地 ShardList 和 Consul 上 ShardList 不一致的节点
}

type CheckUnsynchronizedConfigReuslt struct {
	UnsynchronizedConfigTypes map[uint32]bool `json:"UnsynchronizedConfigTypes"`
}
