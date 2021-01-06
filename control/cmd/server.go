package cmd

import (
	"fmt"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
	"github.com/spf13/cobra"
	"laser-control/apis"
	"laser-control/common"
	"laser-control/context"
	"laser-control/logger"
	"laser-control/middlerware"
	"laser-control/service"
)

func init() {
	rootCmd.AddCommand(serverCmd)
}

var serverCmd = &cobra.Command{
	Use:   "server",
	Short: "Server feature service manager",
	Long:  "Server feature service manager",
	Run: func(cmd *cobra.Command, args []string) {
		ctx := context.New()
		serverMode := ctx.GetConfig().HttpServerMode()
		if serverMode != "" {
			gin.SetMode(serverMode)
		} else {
			gin.SetMode(gin.ReleaseMode)
		}

		intervalClean := ctx.GetConfig().SessionSaveSeconds()
		common.InitCleanTokens(intervalClean, func() {
			userModel := &service.UserModel{Ctx: ctx}
			userModel.CleanTokens(intervalClean)
		})

		// 初始化 module
		service.InitService(ctx)

		// 初始化 server
		app := gin.Default()
		app.Use(middlerware.Cors())
		binding.Validator = new(common.DefaultValidator)
		apis.RegisterRoutes(app, ctx)
		ctx.Log().Info("Start http server", logger.String("host", ctx.GetConfig().HttpServerHost()),
			logger.Int("port", ctx.GetConfig().HttpServerPort()))
		_ = app.Run(fmt.Sprintf("%s:%d", ctx.GetConfig().HttpServerHost(), ctx.GetConfig().HttpServerPort()))
	},
}
