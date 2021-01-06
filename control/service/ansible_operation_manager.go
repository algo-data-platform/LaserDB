package service

import (
	"bytes"
	"io/ioutil"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"path"
	"strconv"
	"sync"

	// "time"

	"gopkg.in/ini.v1"
	"gopkg.in/yaml.v2"

	ansibler "github.com/apenella/go-ansible"
)

const (
	DefaultInventorySectionName string = "hosts"
	AnsiblePlaybookTagsStart    string = "start"
	AnsiblePlaybookTagsStop     string = "stop"
	AnsiblePlaybookTagsRestart  string = "restart"
	AnsiblePlaybookTagsUpgrade  string = "upgrade"
	AnsiblePlaybookTagsEnable   string = "enable"
	AnsiblePlaybookTagsDisable  string = "disable"
)

var SupportedTags = map[string]bool{
	AnsiblePlaybookTagsStart:   true,
	AnsiblePlaybookTagsStop:    true,
	AnsiblePlaybookTagsRestart: true,
	AnsiblePlaybookTagsUpgrade: true,
	AnsiblePlaybookTagsEnable:  true,
	AnsiblePlaybookTagsDisable: true,
}

type AnsiblePlaybookEntity struct {
	Hosts     string            `yaml:"hosts,omitempty"`
	Variables map[string]string `yaml:"vars,omitempty"`
	Roles     []string          `yaml:"roles,omitempty"`
}

type OperationCache struct {
	sync.Mutex
	operationName string
	log           *bytes.Buffer
	done          bool
}

type AnsibleOperationManager struct {
	sync.Mutex
	ctx             *context.Context
	operationCaches map[string]*OperationCache // groupName: OperationCahce => 一个组只允许同时执行一个操作
}

var (
	nodeOperatorManager     *AnsibleOperationManager
	nodeOperatorManagerOnce sync.Once
)

func GetNodeOperatorManager(ctx *context.Context) *AnsibleOperationManager {
	nodeOperatorManagerOnce.Do(func() {
		instance := &AnsibleOperationManager{
			ctx:             ctx,
			operationCaches: make(map[string]*OperationCache),
		}
		nodeOperatorManager = instance
	})
	return nodeOperatorManager
}

func (manager *AnsibleOperationManager) AnsibleOperation(param params.NodeBatchOperationParams) (*common.Status, *params.NodeBatchOperationResult) {
	result := &params.NodeBatchOperationResult{}
	if _, ok := SupportedTags[param.Tags]; !ok {
		return common.StatusError(common.AnsibleUnknownTags), result
	}

	taskId := manager.getOperationKey(param)
	if taskId == "" {
		return common.StatusError(common.AnsibleParamsError), result
	}

	var cache *OperationCache
	manager.Lock()
	cache, ok := manager.operationCaches[taskId]
	manager.Unlock()
	if ok {
		cache.Lock()
		defer cache.Unlock()
		if cache.done == false {
			return common.StatusError(common.AnsibleGroupHasTaskInProgress), result
		}
	}
	result.TaskId = taskId
	go manager.ansibleOperation(param, taskId)

	return common.StatusOk(), result
}

func (manager *AnsibleOperationManager) GetOperationInfo(param params.NodeBatchOperationInfoParams) (*common.Status, *params.NodeBatchOperationInfo) {
	var cache *OperationCache
	info := &params.NodeBatchOperationInfo{}
	ok := false
	manager.Lock()
	cache, ok = manager.operationCaches[param.GroupName]
	manager.Unlock()
	if !ok {
		info.HasTask = false
		return common.StatusOk(), info
	}
	cache.Lock()
	defer cache.Unlock()
	info.OperationName = cache.operationName
	info.Log = string(cache.log.Bytes())
	info.Done = cache.done
	info.HasTask = true
	return common.StatusOk(), info
}

func (manager *AnsibleOperationManager) getOperationKey(param params.NodeBatchOperationParams) string {
	if len(param.Nodes) >= 1 {
		return param.Nodes[0].GroupName
	}
	return ""
}

func (manager *AnsibleOperationManager) ansibleOperation(param params.NodeBatchOperationParams, taskFlag string) *common.Status {
	cache := &OperationCache{
		operationName: param.OperationName,
		log:           new(bytes.Buffer),
		done:          false,
	}
	manager.Lock()
	manager.operationCaches[taskFlag] = cache
	manager.Unlock()

	if status := manager.generateAnsibleInventory(param); !status.Ok() {
		cache.Lock()
		cache.log.Write([]byte(status.Error()))
		cache.done = true
		cache.Unlock()
		return status
	}
	if status := manager.generateAnsiblePlaybook(param); !status.Ok() {
		cache.Lock()
		cache.log.Write([]byte(status.Error()))
		cache.done = true
		cache.Unlock()
		return status
	}

	ansiblePlaybookConnectionOptions := &ansibler.AnsiblePlaybookConnectionOptions{
		Connection: "smart",
	}

	inventoryPath := path.Join(manager.ctx.GetConfig().AnsiblePlaybookDir(), manager.ctx.GetConfig().AnsibleTmpHostFile())
	ansiblePlaybookOptions := &ansibler.AnsiblePlaybookOptions{
		ExtraVars: param.AnsibleVars,
		Inventory: inventoryPath,
		Tags:      param.Tags,
	}

	ansibler.AnsibleForceColor()
	playbookPath := path.Join(manager.ctx.GetConfig().AnsiblePlaybookDir(), manager.ctx.GetConfig().AnsibleTmpPlaybookFile())
	playbook := &ansibler.AnsiblePlaybookCmd{
		Playbook:          playbookPath,
		ConnectionOptions: ansiblePlaybookConnectionOptions,
		Options:           ansiblePlaybookOptions,
		ExecPrefix:        param.OperationName,
		Writer:            cache.log,
	}

	err := playbook.Run()
	cache.Lock()
	defer cache.Unlock()
	cache.done = true
	if err != nil {
		failedInfo := err.Error() + "\n\n Failed!"
		cache.log.Write([]byte(failedInfo))
		return common.StatusWithError(err)
	}
	cache.log.Write([]byte("\n\n Done."))
	return common.StatusOk()
}

func (manager *AnsibleOperationManager) generateAnsibleInventory(param params.NodeBatchOperationParams) *common.Status {
	inventoryPath := path.Join(manager.ctx.GetConfig().AnsiblePlaybookDir(), manager.ctx.GetConfig().AnsibleTmpHostFile())
	ini.DefaultHeader = true
	ini.PrettyFormat = false
	options := ini.LoadOptions{
		Loose:                    true,
		KeyValueDelimiterOnWrite: " ",
		PreserveSurroundedQuote:  false,
	}

	file, err := ini.LoadSources(options, inventoryPath)
	if err != nil {
		return common.StatusWithError(err)
	}
	for _, section := range file.Sections() {
		file.DeleteSection(section.Name())
	}

	newSection, err := file.NewSection(DefaultInventorySectionName)
	if err != nil {
		return common.StatusWithError(err)
	}

	for _, nodeInfo := range param.Nodes {
		key := nodeInfo.Host
		sshPort := manager.ctx.GetConfig().AnsibleSshPort()
		value := "group_name=" + nodeInfo.GroupName +
			" node_id=" + strconv.FormatUint(uint64(nodeInfo.NodeId), 10) +
			" dc=" + nodeInfo.Dc +
			" ansible_ssh_port=" + strconv.FormatUint(uint64(sshPort), 10)
		newSection.NewKey(key, value)
	}
	if err = file.SaveTo(inventoryPath); err != nil {
		return common.StatusWithError(err)
	}
	return common.StatusOk()
}

func (manager *AnsibleOperationManager) generateAnsiblePlaybook(param params.NodeBatchOperationParams) *common.Status {
	playbookPath := path.Join(manager.ctx.GetConfig().AnsiblePlaybookDir(), manager.ctx.GetConfig().AnsibleTmpPlaybookFile())
	playbook := make([]AnsiblePlaybookEntity, 0)
	play := AnsiblePlaybookEntity{
		Hosts: "all",
		Roles: param.Roles,
	}
	playbook = append(playbook, play)

	data, err := yaml.Marshal(&playbook)
	if err != nil {
		return common.StatusWithError(err)
	}

	if err = ioutil.WriteFile(playbookPath, data, 0644); err != nil {
		return common.StatusWithError(err)
	}

	return common.StatusOk()
}
