package apis

import (
	"fmt"
	"laser-control/context"
	_ "laser-control/docs"
	"laser-control/middlerware"

	"github.com/gin-gonic/gin"
	swaggerFiles "github.com/swaggo/files"
	ginSwagger "github.com/swaggo/gin-swagger"
)

// @title Laser Control API
// @version 1.0
// @description 主要提供 Laser 集群管理相关 API

// @contact.name API Support
// @BasePath /
func RegisterRoutes(app *gin.Engine, ctx *context.Context) {
	app.LoadHTMLGlob(fmt.Sprintf("%s/frontend/dist/*.html", ctx.GetConfig().StaticRoot()))      // 添加入口index.html
	app.Static("/static", fmt.Sprintf("%s/frontend/dist/static", ctx.GetConfig().StaticRoot())) // 添加资源路径
	app.StaticFile("/", fmt.Sprintf("%s/frontend/dist/index.html", ctx.GetConfig().StaticRoot()))

	url := ginSwagger.URL("doc.json") // The url pointing to API definition
	app.GET("/swagger/*any", ginSwagger.WrapHandler(swaggerFiles.Handler, url))
	databasePrefix := app.Group("/database")
	databasePrefix.Use(middlerware.Auth(ctx))
	{
		DBStore(databasePrefix, ctx)
		DBList(databasePrefix, ctx)
		DBUpdate(databasePrefix, ctx)
		DBDelete(databasePrefix, ctx)
	}

	tablePrefix := app.Group("/table")
	tablePrefix.Use(middlerware.Auth(ctx))
	{
		TableStore(tablePrefix, ctx)
		TableList(tablePrefix, ctx)
		TableListDetail(tablePrefix, ctx)
		TableUpdate(tablePrefix, ctx)
		TableDelete(tablePrefix, ctx)
		TableSynchronizeToConsul(tablePrefix, ctx)
		TableListAllCommands(tablePrefix, ctx)
	}
	nodeConfigPrefix := app.Group("/node_config")
	nodeConfigPrefix.Use(middlerware.Auth(ctx))
	{
		NodeConfigStore(nodeConfigPrefix, ctx)
		NodeConfigList(nodeConfigPrefix, ctx)
		NodeConfigUpdate(nodeConfigPrefix, ctx)
		NodeConfigDelete(nodeConfigPrefix, ctx)
	}
	tableConfigPrefix := app.Group("/table_config")
	tableConfigPrefix.Use(middlerware.Auth(ctx))
	{
		TableConfigStore(tableConfigPrefix, ctx)
		TableConfigList(tableConfigPrefix, ctx)
		TableConfigListWithDetailItems(tableConfigPrefix, ctx)
		TableConfigUpdate(tableConfigPrefix, ctx)
		TableConfigDelete(tableConfigPrefix, ctx)
	}

	proxyTableConfigPrefix := app.Group("/proxy_config")
	proxyTableConfigPrefix.Use(middlerware.Auth(ctx))
	{
		ProxyTableConfigStore(proxyTableConfigPrefix, ctx)
		ProxyTableConfigList(proxyTableConfigPrefix, ctx)
		ProxyTableConfigUpdate(proxyTableConfigPrefix, ctx)
		ProxyTableConfigSynchronizeToConsul(proxyTableConfigPrefix, ctx)
	}

	keycheckPrefix := app.Group("/keycheck")
	keycheckPrefix.Use(middlerware.Auth(ctx))
	{
		KeyCheck(keycheckPrefix, ctx)
	}

	userPrefix := app.Group("/user")
	userPrefix.Use(middlerware.Auth(ctx))
	{
		UserInfo(userPrefix, ctx)
	}
	servicePrefix := app.Group("/service")
	servicePrefix.Use(middlerware.Auth(ctx))
	{
		ServiceStore(servicePrefix, ctx)
	}

	dcPrefix := app.Group("/dc")
	dcPrefix.Use(middlerware.Auth(ctx))
	{
		DcStore(dcPrefix, ctx)
		DcUpdate(dcPrefix, ctx)
		DcList(dcPrefix, ctx)
	}

	clusterPrefix := app.Group("/cluster")
	clusterPrefix.Use(middlerware.Auth(ctx))
	{
		ClusterStore(clusterPrefix, ctx)
		ClusterList(clusterPrefix, ctx)
		ClusterUpdate(clusterPrefix, ctx)
		ClusterSynchronizeToConsul(clusterPrefix, ctx)
	}

	groupPrefix := app.Group("/group")
	groupPrefix.Use(middlerware.Auth(ctx))
	{
		GroupStore(groupPrefix, ctx)
		GroupList(groupPrefix, ctx)
		GroupUpdate(groupPrefix, ctx)
		GroupDelete(groupPrefix, ctx)
		GroupReduceMetrics(groupPrefix, ctx)
	}

	nodePrefix := app.Group("/node")
	nodePrefix.Use(middlerware.Auth(ctx))
	{
		NodeStore(nodePrefix, ctx)
		NodeBatchStore(nodePrefix, ctx)
		NodeUpdate(nodePrefix, ctx)
		NodeChangeRole(nodePrefix, ctx)
		NodeDelete(nodePrefix, ctx)
		NodeBatchDelete(nodePrefix, ctx)
		NodeList(nodePrefix, ctx)
		ShardList(nodePrefix, ctx)
		AnsibleOperation(nodePrefix, ctx)
		GetAnsbileOperationInfo(nodePrefix, ctx)
	}

	nodePhysicalMetricsPrefix := app.Group("/node_physical_metric")
	nodePhysicalMetricsPrefix.Use(middlerware.Auth(ctx))
	{
		NodePhysicalMetricsInsert(nodePhysicalMetricsPrefix, ctx)
		NodePhysicalMetricsList(nodePhysicalMetricsPrefix, ctx)
		NodePhysicalMetricsNodeList(nodePhysicalMetricsPrefix, ctx)
	}

	nodeRunningMetricsPrefix := app.Group("/node_running_metric")
	nodePhysicalMetricsPrefix.Use(middlerware.Auth(ctx))
	{
		NodeRunningMetricsInsert(nodeRunningMetricsPrefix, ctx)
		NodeRunningMetricsList(nodeRunningMetricsPrefix, ctx)
		NodeRunningMetricsNodeList(nodeRunningMetricsPrefix, ctx)
	}

	groupRunningMetricsPrefix := app.Group("/group_running_metric")
	nodePhysicalMetricsPrefix.Use(middlerware.Auth(ctx))
	{
		GroupRunningMetricsInsert(groupRunningMetricsPrefix, ctx)
		GroupRunningMetricsList(groupRunningMetricsPrefix, ctx)
		GroupRunningMetricsGroupList(groupRunningMetricsPrefix, ctx)
	}

	resourcePoolRunningMetricsPrefix := app.Group("resource_pool_running_metric")
	nodePhysicalMetricsPrefix.Use(middlerware.Auth(ctx))
	{
		ResourcePoolRunningMetricsInsert(resourcePoolRunningMetricsPrefix, ctx)
		ResourcePoolRunningMetricsList(resourcePoolRunningMetricsPrefix, ctx)
		ResourcePoolRunningMetricsPoolList(resourcePoolRunningMetricsPrefix, ctx)
	}

	clusterRunningMetricsPrefix := app.Group("cluster_running_metric")
	nodePhysicalMetricsPrefix.Use(middlerware.Auth(ctx))
	{
		ClusterRunningMetricsInsert(clusterRunningMetricsPrefix, ctx)
		ClusterRunningMetricsList(clusterRunningMetricsPrefix, ctx)
	}

	tableMetricsPrefix := app.Group("table_metric")
	nodePhysicalMetricsPrefix.Use(middlerware.Auth(ctx))
	{
		TableMetricsInsert(tableMetricsPrefix, ctx)
		TableMetricsList(tableMetricsPrefix, ctx)
		TableMetricsTableList(tableMetricsPrefix, ctx)
	}

	systemIndexPrefix := app.Group("system_index")
	nodePhysicalMetricsPrefix.Use(middlerware.Auth(ctx))
	{
		SystemIndexInsert(systemIndexPrefix, ctx)
		SystemIndexList(systemIndexPrefix, ctx)
	}

	versionChangePrefix := app.Group("/version_change")
	versionChangePrefix.Use(middlerware.Auth(ctx))
	{
		VersionChangeShow(versionChangePrefix, ctx)
		VersionChangeClear(versionChangePrefix, ctx)
		VersionChangeVersion(versionChangePrefix, ctx)
		VersionChangeRollback(versionChangePrefix, ctx)
	}

	toolsPrefix := app.Group("/tools")
	toolsPrefix.Use(middlerware.Auth(ctx))
	{
		AssignTableShardList(toolsPrefix, ctx)
	}

	machineCategoryPrefix := app.Group("/machine_category")
	machineCategoryPrefix.Use(middlerware.Auth(ctx))
	{
		MachineCategoryStore(machineCategoryPrefix, ctx)
		MachineCategoryList(machineCategoryPrefix, ctx)
		MachineCategoryUpdate(machineCategoryPrefix, ctx)
		MachineCategoryDelete(machineCategoryPrefix, ctx)
	}

	machinePrefix := app.Group("/machine")
	machinePrefix.Use(middlerware.Auth(ctx))
	{
		MachineStore(machinePrefix, ctx)
		MachineList(machinePrefix, ctx)
		MachineUpdate(machinePrefix, ctx)
		MachineDelete(machinePrefix, ctx)
	}

	ansibleConfigPrefix := app.Group("/ansible_config")
	ansibleConfigPrefix.Use(middlerware.Auth(ctx))
	{
		AnsibleConfigStore(ansibleConfigPrefix, ctx)
		AnsibleConfigList(ansibleConfigPrefix, ctx)
		AnsibleConfigUpdate(ansibleConfigPrefix, ctx)
		AnsibleConfigDelete(ansibleConfigPrefix, ctx)
	}

	ticketPrefix := app.Group("/ticket")
	ticketPrefix.Use(middlerware.Auth(ctx))
	{
		TicketStcore(ticketPrefix, ctx)
		TicketList(ticketPrefix, ctx)
		TicketUpdate(ticketPrefix, ctx)
		TicketProcess(ticketPrefix, ctx)
		TicketDelete(ticketPrefix, ctx)
	}

	validatorPrefix := app.Group("/config_validator")
	validatorPrefix.Use(middlerware.Auth(ctx))
	{
		CheckGroupReadyToBeMaster(validatorPrefix, ctx)
		CheckUnsynchronizedConfig(validatorPrefix, ctx)
	}
}
