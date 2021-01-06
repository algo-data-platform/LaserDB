package context

import (
	"github.com/spf13/viper"
)

type Config struct {
	debug                   bool
	httpServerHost          string
	httpServerPort          int
	httpServerMode          string
	staticRoot              string
	databaseDriver          string
	databaseDsn             string
	sessionSaveSeconds      int
	consulAddress           string
	consulToken             string
	consulScheme            string
	serviceName             string
	proxyServiceName        string
	emailUser               string
	emailPassword           string
	emailSnmpAddress        string
	emailFrom               string
	commonUrls              []string
	adminUsers              []string
	projectName             string
	laserClientTimeout      int
	singleOperations        []string
	multipleOperations      []string
	batchManagerUrl         string
	ansibleSshPort          uint16
	ansiblePlaybookDir      string
	ansibleTmpHostFile      string
	ansibleTmpPlaybookFile  string
	serverPrometheusAddress string
	clientPrometheusAddress string
	cronExpressionPeak      string
	cronExpressionPeacePeak string
}

func NewConfig() *Config {
	c := &Config{}
	c.httpServerHost = viper.GetString("http.host")
	c.httpServerPort = viper.GetInt("http.port")
	if viper.GetBool("debug") {
		c.httpServerMode = "debug"
	}
	c.staticRoot = viper.GetString("static.root")
	c.databaseDriver = viper.GetString("database.driver")
	c.databaseDsn = viper.GetString("database.dsn")
	c.sessionSaveSeconds = viper.GetInt("auth.sessionSaveSeconds")
	c.consulAddress = viper.GetString("consul.address")
	c.consulToken = viper.GetString("consul.token")
	c.consulScheme = viper.GetString("sonul.scheme")
	c.serviceName = viper.GetString("cluster.serviceName")
	c.proxyServiceName = viper.GetString("cluster.proxyServiceName")
	c.emailUser = viper.GetString("email.emailUser")
	c.emailPassword = viper.GetString("email.emailPassword")
	c.emailSnmpAddress = viper.GetString("email.emailSnmpAddress")
	c.emailFrom = viper.GetString("email.emailFrom")
	c.commonUrls = viper.GetStringSlice("commonUrls")
	c.adminUsers = viper.GetStringSlice("adminUsers")
	c.projectName = viper.GetString("project.projectName")
	c.laserClientTimeout = viper.GetInt("laserClient.timeoutMs")
	c.singleOperations = viper.GetStringSlice("laserCommands.singleOperations")
	c.multipleOperations = viper.GetStringSlice("laserCommands.multipleOperations")
	c.batchManagerUrl = viper.GetString("batchUpdateManager.url")
	c.ansibleSshPort = uint16(viper.GetUint("ansible.sshPort"))
	c.ansiblePlaybookDir = viper.GetString("ansible.playbookDir")
	c.ansibleTmpHostFile = viper.GetString("ansible.tmpHostFile")
	c.ansibleTmpPlaybookFile = viper.GetString("ansible.tmpPlaybookFile")
	c.serverPrometheusAddress = viper.GetString("prometheus.serverPrometheusAddress")
	c.clientPrometheusAddress = viper.GetString("prometheus.clientPrometheusAddress")
	c.cronExpressionPeak = viper.GetString("report.cronExpressionPeak")
	c.cronExpressionPeacePeak = viper.GetString("report.cronExpressionPeacePeak")

	return c
}

func (c *Config) HttpServerHost() string {
	return c.httpServerHost
}

func (c *Config) HttpServerPort() int {
	return c.httpServerPort
}

func (c *Config) HttpServerMode() string {
	return c.httpServerMode
}

func (c *Config) StaticRoot() string {
	return c.staticRoot
}

func (c *Config) DatabaseDriver() string {
	return c.databaseDriver
}

func (c *Config) DatabaseDsn() string {
	return c.databaseDsn
}

func (c *Config) Debug() bool {
	return "debug" == c.httpServerMode
}

func (c *Config) SessionSaveSeconds() int32 {
	return int32(c.sessionSaveSeconds)
}

func (c *Config) ConsulAddress() string {
	return c.consulAddress
}

func (c *Config) ConsulScheme() string {
	return c.consulScheme
}

func (c *Config) ConsulToken() string {
	return c.consulToken
}

func (c *Config) ServiceName() string {
	return c.serviceName
}

func (c *Config) ProxyServiceName() string {
	return c.proxyServiceName
}

func (c *Config) EmailUser() string {
	return c.emailUser
}

func (c *Config) EailPassword() string {
	return c.emailPassword
}

func (c *Config) EmailSnmpAddress() string {
	return c.emailSnmpAddress
}

func (c *Config) EmailFrom() string {
	return c.emailFrom
}

func (c *Config) CommonUrls() []string {
	return c.commonUrls
}

func (c *Config) AdminUsers() []string {
	return c.adminUsers
}

func (c *Config) ProjectName() string {
	return c.projectName
}

func (c *Config) LaserClientTimeout() int {
	return c.laserClientTimeout
}

func (c *Config) SingleOperations() []string {
	return c.singleOperations
}

func (c *Config) MultipleOperations() []string {
	return c.multipleOperations
}

func (c *Config) BatchManagerUrl() string {
	return c.batchManagerUrl
}

func (c *Config) AnsibleSshPort() uint16 {
	return c.ansibleSshPort
}

func (c *Config) AnsiblePlaybookDir() string {
	return c.ansiblePlaybookDir
}

func (c *Config) AnsibleTmpHostFile() string {
	return c.ansibleTmpHostFile
}

func (c *Config) AnsibleTmpPlaybookFile() string {
	return c.ansibleTmpPlaybookFile
}

func (c *Config) ServerPrometheusAddress() string {
	return c.serverPrometheusAddress
}

func (c *Config) ClientPrometheusAddress() string {
	return c.clientPrometheusAddress
}

func (c *Config) ReportCronExpressionPeak() string {
	return c.cronExpressionPeak
}

func (c *Config) ReportCronExpressionPeacePeak() string {
	return c.cronExpressionPeacePeak
}
