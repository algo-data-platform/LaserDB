# Laser Control

## 预置条件
如果环境没有初始化可能会下载依赖，由于很多依赖被墙，所以需要设置代理：
export GOPROXY=https://goproxy.io

### 开发接口自动生成文档
1. 环境需要安装 swag
`go get -u github.com/swaggo/swag/cmd/swag@v1.6.3`
2. 生成文档配置信息
swag init -g apis/router.go
3. 运行项目
  3.1 运行
	go run main.go server --config=config/config.yml

## 线上部署

1. 运行 build.sh 打包新版本
 sh build.sh [commit-id] prod [control]

 2. 设置部署参数
    2.1 cd deploy
    2.2 如果 config.yml 里的设置参数有删减，需要在 roles/laser_control/templates/config.yml 同步修改；
    2.3 设置部署参数,修改 laser_demo_control.yml，其中pakcage_tag 改成步骤1 中的 commit-id，其他参数按需修改;
    
 3. 部署上线
	 	cd deploy
	  sudo su adbot
	  ansible-playbook -i hosts laser_demo_control.yml --tags upgrade
	
 4. 验证上线结果
    浏览器 访问 http://host:port 验证上线结果
