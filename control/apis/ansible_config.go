package apis

import (
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"laser-control/service"

	"github.com/gin-gonic/gin"
)

// @Summary 添加Ansible配置
// @Tags ansible_config
// @Accept  json
// @Produce  json
// @Param data body params.AnsibleConfigParams true "Ansible配置信息"
// @Success 200 {object} apis.Result
// @Router /ansible_config/store [post]
func AnsibleConfigStore(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/store", func(c *gin.Context) {
		var param params.AnsibleConfigParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		ansibleConfig := service.NewAnsibleConfigModel(ctx)
		status := ansibleConfig.Store(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 查询Ansible配置
// @Tags ansible_config
// @Accept  json
// @Produce  json
// @Param Page query int true "页数"
// @Param Limit query int true "个数"
// @Success 200 {object} apis.Result
// @Router /ansible_config/list [get]
func AnsibleConfigList(router *gin.RouterGroup, ctx *context.Context) {
	router.GET("/list", func(c *gin.Context) {
		var param params.AnsibleConfigListParams
		if err := c.ShouldBindQuery(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}

		ansibleConfig := service.NewAnsibleConfigModel(ctx)
		list, total := ansibleConfig.List(param, true)
		result := make([]params.AnsibleConfigParams, 0)
		for _, info := range list {
			result = append(result, params.AnsibleConfigParams{
				Id:    info.Id,
				Name:  info.Name,
				Vars:  info.Vars,
				Roles: info.Roles,
				Desc:  info.Desc,
			})
		}
		ReturnData(c, ListResult{
			Total: total,
			Items: result,
		})
	})
}

// @Summary 更新Ansible配置
// @Tags ansible_config
// @Accept  json
// @Produce  json
// @Param data body params.AnsibleConfigParams true "Ansible配置信息"
// @Success 200 {object} apis.Result
// @Router /ansible_config/update [post]
func AnsibleConfigUpdate(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/update", func(c *gin.Context) {
		var param params.AnsibleConfigParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		ansibleConfig := service.NewAnsibleConfigModel(ctx)
		status := ansibleConfig.Update(param)
		ReturnJson(c, status, nil)
	})
}

// @Summary 删除Ansible配置
// @Tags ansible_config
// @Accept  json
// @Produce  json
// @Param data body params.AnsibleConfigDeleteParams true "Ansible配置ID"
// @Success 200 {object} apis.Result
// @Router /ansible_config/delete [post]
func AnsibleConfigDelete(router *gin.RouterGroup, ctx *context.Context) {
	router.POST("/delete", func(c *gin.Context) {
		var param params.AnsibleConfigDeleteParams
		if err := c.ShouldBindJSON(&param); err != nil {
			ReturnError(c, common.StatusWithMessage(common.ParamsInvalid, err.Error()))
			return
		}
		ansibleConfig := service.NewAnsibleConfigModel(ctx)
		status := ansibleConfig.Delete(param)
		ReturnJson(c, status, nil)
	})
}
