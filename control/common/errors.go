package common

import (
	"fmt"
	"path/filepath"
	"runtime"
)

const (
	OK    = 0
	ERROR = 1

	ParamsInvalid    = 100
	HttpRequestError = 1001

	ServiceDup = 10001

	FeatureNameDup = 11001

	ViewNameDup  = 12001
	ViewNotFound = 12002

	TableNameDup   = 13001
	TableNotExists = 13002

	DatabaseNameDup   = 14001
	DatabaseNotExists = 14002
	DatabaseInUse     = 14003

	ConsulLoadJsonFailed = 15001
	ConsulPutKVFailed    = 15002
	ConsulGetKVFailed    = 15003

	GetServerListFailed      = 16001
	GetValueFromServerFailed = 16002

	TableConfigNameDup   = 170001
	TableConfigNotExists = 170002
	TableConfigInUse     = 170003

	TokenInvalid     = 20001
	TokenExpired     = 20002
	PermissionDenied = 20003

	ClusterParserError = 30001
	ClusterNameDup     = 30002
	ClusterNotExist    = 30003

	DcNameDup  = 31001
	DcNotExist = 31002

	ShardLeaderDup       = 40001
	ShardVersionNotFound = 40002

	GroupNameDup  = 50001
	GroupNotExist = 50002
	GroupInUse    = 50003

	VersionChangeApiError = 60001

	NodeDup                     = 70001
	NodeAddressDup              = 70002
	NodeIdDup                   = 70003
	NodeNotExist                = 70004
	NodeSetEdgeNodeAsMasterFail = 70005
	NodeConfigInUse             = 70006

	GetInfoFailed  = 80001
	ShardDupFailed = 80002

	AssignTableShardListNumTooLarge = 90001

	MachineCategoryDup                     = 91001
	MachineCategoryNotExists               = 91002
	MachineCategoryDeleteWhenMachineExists = 91003

	MachineDup       = 92001
	MachineNotExists = 92002
	MachineInUse     = 92003

	AnsibleConfigDup       = 93001
	AnsibleConfigNotExists = 93002
	AnsibleConfigInUse     = 93003

	AnsibleUnknownTags            = 100001
	AnsibleGroupHasNoTask         = 100002
	AnsibleGroupHasTaskInProgress = 100003
	AnsibleParamsError            = 100004

	TicketDup         = 110001
	TicketNotExists   = 110002
	TicketCanotHandle = 110003

	ReportQueryPrometheusFailed        = 120001
	ReportPrometheusEmptyResult        = 120002
	ReportPrometheusInvalidValueStruct = 120003
	ReportPrometheusInvalidValueType   = 120004
	ReportPrometheusInvalidResponse    = 120005
)

var statusMessages map[uint32]string

func init() {
	statusMessages = make(map[uint32]string)
	statusMessages[OK] = "OK"
	statusMessages[ERROR] = "ERROR"

	// 系统通用相关
	statusMessages[ParamsInvalid] = "参数不合法"

	// service 相关错误
	statusMessages[ServiceDup] = "服务已经存在"

	// feature 相关错误
	statusMessages[FeatureNameDup] = "特征名称已经存在"

	// view 相关错误
	statusMessages[ViewNameDup] = "特征视图已经存在"
	statusMessages[ViewNotFound] = "特征视图不存在"

	// table 相关错误
	statusMessages[TableNameDup] = "数据表已经存在"
	statusMessages[TableNotExists] = "数据表不存在"

	// database 相关错误
	statusMessages[DatabaseNameDup] = "数据库已经存在"
	statusMessages[DatabaseNotExists] = "数据库不存在"
	statusMessages[DatabaseInUse] = "数据库正在使用中"

	// consul 相关错误
	statusMessages[ConsulLoadJsonFailed] = "生成Json字符串失败"
	statusMessages[ConsulPutKVFailed] = "上传 Consule KV 失败"
	statusMessages[ConsulGetKVFailed] = "获取 Consule KV 失败"

	// table config 相关错误
	statusMessages[TableConfigNameDup] = "表配置已经存在"
	statusMessages[TableConfigNotExists] = "表配置不存在"
	statusMessages[TableConfigInUse] = "表配置正在使用中"

	statusMessages[TokenInvalid] = "非法 Token"
	statusMessages[TokenExpired] = "Token 已过期"
	statusMessages[PermissionDenied] = "没有权限"

	statusMessages[ClusterParserError] = "集群配置文件解析错误"
	statusMessages[ClusterNameDup] = "集群名字重复"
	statusMessages[ClusterNotExist] = "集群不存在"

	statusMessages[DcNameDup] = "数据中心名字重复"
	statusMessages[DcNotExist] = "数据中心不存在"

	statusMessages[GroupNameDup] = "组名字重复"
	statusMessages[GroupNotExist] = "组不存在"
	statusMessages[GroupInUse] = "组正在使用中"

	statusMessages[NodeDup] = "节点已经存在"
	statusMessages[NodeAddressDup] = "节点地址重复"
	statusMessages[NodeIdDup] = "节点ID重复"
	statusMessages[NodeNotExist] = "节点不存在"
	statusMessages[NodeSetEdgeNodeAsMasterFail] = "不允许将边缘节点设置为主节点"
	statusMessages[NodeConfigInUse] = "节点配置正在使用中"

	statusMessages[GetInfoFailed] = "获取信息失败"
	statusMessages[ShardDupFailed] = "shard降级失败"

	statusMessages[AssignTableShardListNumTooLarge] = "要分配的分片列表个数大于分片个数"

	statusMessages[MachineCategoryDup] = "机器资源分类已经存在"
	statusMessages[MachineCategoryNotExists] = "机器资源分类不存在"
	statusMessages[MachineCategoryDeleteWhenMachineExists] = "当前机器资源分类下仍有机器，不能删除"

	statusMessages[MachineDup] = "机器资源已经存在"
	statusMessages[MachineNotExists] = "机器资源不存在"
	statusMessages[MachineInUse] = "机器正在被使用中"

	statusMessages[AnsibleConfigDup] = "Ansible 配置已经存在"
	statusMessages[AnsibleConfigNotExists] = "Ansible 配置不存在"
	statusMessages[AnsibleConfigInUse] = "Ansible 正在使用中"

	statusMessages[AnsibleUnknownTags] = "不支持的 AnsiblePlaybook Tags"
	statusMessages[AnsibleGroupHasNoTask] = "没有当前组的任务信息"
	statusMessages[AnsibleGroupHasTaskInProgress] = "当前组仍有任务在执行中"
	statusMessages[AnsibleParamsError] = "参数错误"

	statusMessages[TicketDup] = "工单已经存在"
	statusMessages[TicketNotExists] = "工单不存在"
	statusMessages[TicketCanotHandle] = "没有权限处理"

	statusMessages[ReportQueryPrometheusFailed] = "Prometheus 查询失败"
	statusMessages[ReportPrometheusEmptyResult] = "Prometheus 结果为空"
	statusMessages[ReportPrometheusInvalidValueStruct] = "Promethues 无效的 Value 结构"
	statusMessages[ReportPrometheusInvalidValueType] = "Promethues 无效的 Value 类型"
	statusMessages[ReportPrometheusInvalidResponse] = "Promethues 无效的 Response"
}

type Status struct {
	code    uint32
	message string
	where   string
}

func StatusOk() *Status {
	return &Status{
		code:    OK,
		message: "",
		where:   "",
	}
}

func StatusError(code uint32) *Status {
	message, ok := statusMessages[code]
	if !ok {
		message = "未知错误"
	}

	return &Status{
		code:    code,
		message: message,
		where:   caller(1, false),
	}
}

func StatusWithMessage(code uint32, message string) *Status {
	return &Status{
		code:    code,
		message: message,
		where:   caller(1, false),
	}
}

func StatusWithError(err error) *Status {
	code := OK
	msg := ""
	if err != nil {
		code = ERROR
		msg = err.Error()
	}
	return &Status{
		code:    uint32(code),
		message: msg,
		where:   "",
	}
}

func StatusErrorWhere(code uint32, message string, callDepth int) *Status {
	return &Status{
		code:    code,
		message: message,
		where:   caller(callDepth, false),
	}
}

func (status *Status) Ok() bool {
	return status.code == OK
}

func (status *Status) Code() uint32 {
	return status.code
}

func (status *Status) Error() string {
	return status.message
}

func (status *Status) Where() string {
	return status.where
}

func caller(calldepth int, short bool) string {
	_, file, line, ok := runtime.Caller(calldepth + 1)
	if !ok {
		file = "???"
		line = 0
	} else if short {
		file = filepath.Base(file)
	}

	return fmt.Sprintf("%s:%d", file, line)
}
