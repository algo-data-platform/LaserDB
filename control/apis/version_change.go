package apis

import (
	"laser-control/common"
	"laser-control/common/curl"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"encoding/json"
	"strconv"

	"github.com/gin-gonic/gin"
)

type VersionChangeResult struct {
	Code    uint32      `json:"Code"`
	Message string      `json:"Message"`
	Data    interface{} `json:"Data"`
}

const VersionChangeShowUri string = "/version_change/show"
const VersionChangeClearUri string = "/version_change/clear"
const VersionChangeVersionUri string = "/version_change/version"
const VersionChangeRollbackUri string = "/version_change/rollback"

// @Summary 版本变更记录
// @Tags version_change
// @Accept  json
// @Produce  json
// @Success 200 {object} apis.Result
// @Router /version_change/show [get]
func VersionChangeShow(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/show", func(c *gin.Context) {
		url := ctx.GetConfig().BatchManagerUrl() + VersionChangeShowUri

		req := curl.NewRequest()
		resp, err := req.SetUrl(url).Get()

		if err != nil {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, err.Error()))
			return
		}

		versionChangeStr := string(resp.Body)
		if !resp.IsOk() {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, "VersionChangeShow Api Isok Err:"+versionChangeStr))
			return
		}
		var versionChange VersionChangeResult
		if err := json.Unmarshal([]byte(versionChangeStr), &versionChange); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		if versionChange.Code != 0 {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, versionChange.Message))
			return
		}

		ReturnData(c, versionChange.Data)

	})
}

// @Summary 清空表数据，新建版本
// @Tags version_change
// @Accept  json
// @Produce  json
// @Param data body params.VersionChangeClear true "版本信息"
// @Success 200 {object} apis.Result
// @Router /version_change/clear [post]
func VersionChangeClear(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/clear", func(c *gin.Context) {
		var param params.VersionChangeClear
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		url := ctx.GetConfig().BatchManagerUrl() + VersionChangeClearUri

		databaseMod := service.NewLaserDatabaseModel(ctx)
		status := databaseMod.CheckDupName(param.DatabaseName)
		//数据库不存在
		if status.Ok() {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, "database not vaild"))
			return
		}
		tableMod := service.NewTableModel(ctx)
		tableResult := tableMod.FindOne(param.DatabaseId, param.TableName)

		if tableResult.Id == 0 {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, "table not vaild"))
			return
		}

		if tableResult.PartitionNumber == 0 {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, "table partitionNumber vaild"))
			return
		}

		PartitionNumber := strconv.Itoa(int(tableResult.PartitionNumber))

		queryMap := make(map[string]string)
		queryMap["database_name"] = param.DatabaseName
		queryMap["table_name"] = param.TableName
		queryMap["partition_num"] = string(PartitionNumber)

		req := curl.NewRequest()
		resp, err := req.SetUrl(url).SetQueries(queryMap).Get()

		if err != nil {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, err.Error()))
			return
		}

		versionChangeStr := string(resp.Body)
		if !resp.IsOk() {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, "VersionChangeClear Api Isok Err"+versionChangeStr))
			return
		}
		var versionChange VersionChangeResult
		if err := json.Unmarshal([]byte(versionChangeStr), &versionChange); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		if versionChange.Code != 0 {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, versionChange.Message))
			return
		}

		ReturnData(c, versionChange.Data)

	})
}

// @Summary 当前表版本列表
// @Tags version_change
// @Accept  json
// @Produce  json
// @Param data body params.VersionChangeVersion true "版本信息"
// @Success 200 {object} apis.Result
// @Router /version_change/version [post]
func VersionChangeVersion(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/version", func(c *gin.Context) {
		var param params.VersionChangeVersion
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		url := ctx.GetConfig().BatchManagerUrl() + VersionChangeVersionUri

		databaseMod := service.NewLaserDatabaseModel(ctx)
		status := databaseMod.CheckDupName(param.DatabaseName)
		//数据库不存在
		if status.Ok() {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, "database not vaild"))
			return
		}
		tableMod := service.NewTableModel(ctx)
		tableResult := tableMod.FindOne(param.DatabaseId, param.TableName)

		if tableResult.Id == 0 {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, "table not vaild"))
			return
		}

		queryMap := make(map[string]string)
		queryMap["database_name"] = param.DatabaseName
		queryMap["table_name"] = param.TableName

		req := curl.NewRequest()
		resp, err := req.SetUrl(url).SetQueries(queryMap).Get()

		if err != nil {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, err.Error()))
			return
		}

		versionChangeStr := string(resp.Body)
		if !resp.IsOk() {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, "VersionChangeVersion Api Isok Err"+versionChangeStr))
			return
		}
		var versionChange VersionChangeResult
		if err := json.Unmarshal([]byte(versionChangeStr), &versionChange); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		if versionChange.Code != 0 {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, versionChange.Message))
			return
		}

		ReturnData(c, versionChange.Data)
	})
}

// @Summary 回滚表版本数据
// @Tags version_change
// @Accept  json
// @Produce  json
// @Param data body params.VersionChangeRollback true "版本信息"
// @Success 200 {object} apis.Result
// @Router /version_change/rollback [post]
func VersionChangeRollback(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/rollback", func(c *gin.Context) {
		var param params.VersionChangeRollback
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		url := ctx.GetConfig().BatchManagerUrl() + VersionChangeRollbackUri

		databaseMod := service.NewLaserDatabaseModel(ctx)
		status := databaseMod.CheckDupName(param.DatabaseName)
		//数据库不存在
		if status.Ok() {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, "database not vaild"))
			return
		}
		tableMod := service.NewTableModel(ctx)
		tableResult := tableMod.FindOne(param.DatabaseId, param.TableName)

		if tableResult.Id == 0 {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, "table not vaild"))
			return
		}

		if tableResult.PartitionNumber == 0 {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, "table partitionNumber vaild"))
			return
		}

		PartitionNumber := strconv.Itoa(int(tableResult.PartitionNumber))

		queryMap := make(map[string]string)
		queryMap["database_name"] = param.DatabaseName
		queryMap["table_name"] = param.TableName
		queryMap["partition_num"] = string(PartitionNumber)
		queryMap["version"] = param.Version

		req := curl.NewRequest()
		resp, err := req.SetUrl(url).SetQueries(queryMap).Get()

		if err != nil {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, err.Error()))
			return
		}

		versionChangeStr := string(resp.Body)
		if !resp.IsOk() {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, "VersionChangeRollback Api Isok Err"+versionChangeStr))
			return
		}
		var versionChange VersionChangeResult
		if err := json.Unmarshal([]byte(versionChangeStr), &versionChange); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		if versionChange.Code != 0 {
			ReturnError(c, common.StatusWithMessage(common.VersionChangeApiError, versionChange.Message))
			return
		}

		ReturnData(c, versionChange.Data)
	})
}
