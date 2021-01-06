package service

import (
	"encoding/xml"
	"fmt"
	"io/ioutil"
	"laser-control/common"
	"laser-control/context"
	"laser-control/params"
	"net/http"
	"sync"
	"time"
)

var sessions sync.Map

type UserModel struct {
	Ctx *context.Context
}

type UserToken struct {
	UserInfo   *params.UserInfoDto
	CreateTime time.Time
}

func NewUserModel(ctx *context.Context) *UserModel {
	instance := &UserModel{
		Ctx: ctx,
	}
	return instance
}

type XmlInfo struct {
	UserName string `xml:"username"`
	Email    string `xml:"email"`
	Uid      string `xml:"uid"`
	Name     string `xml:"name"`
}

type XmlUser struct {
	Info XmlInfo `xml:"info"`
}

const ErpCasValidUrl = "http://your.validate.url/"

func (user *UserModel) Info(token string, service string) (*params.UserInfoDto, *common.Status) {
	var userInfo params.UserInfoDto
	if v, ok := sessions.Load(token); ok {
		userInfo = *v.(UserToken).UserInfo
		return &userInfo, common.StatusOk()
	}

	url := fmt.Sprintf("%s?ticket=%s&codetype=%s&service=%s", ErpCasValidUrl, token, "utf8", service)
	user.Ctx.Log().Info(url)
	resp, err := http.Get(url)
	if err != nil {
		user.Ctx.Log().Info(err.Error())
		return &userInfo, common.StatusWithError(err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return &userInfo, common.StatusWithError(err)
	}
	var xmlUser XmlUser
	err = xml.Unmarshal(body, &xmlUser)
	if err != nil {
		return &userInfo, common.StatusWithError(err)
	}

	user.Ctx.Log().Info(string(body))
	userInfo.Name = xmlUser.Info.Name
	userInfo.Email = xmlUser.Info.Email
	userInfo.Uid = xmlUser.Info.UserName
	userInfo.IsAdmin = user.isAdminUser(userInfo.Email)
	if userInfo.Name == "" {
		return &userInfo, common.StatusError(common.ERROR)
	}

	userToken := UserToken{
		UserInfo:   &userInfo,
		CreateTime: time.Now(),
	}
	sessions.Store(token, userToken)
	return &userInfo, common.StatusOk()
}

func (user *UserModel) CleanTokens(interval int32) {
	var deleteKeys []string
	sessions.Range(func(key, value interface{}) bool {
		userToken := value.(UserToken)

		limitLifeTime := userToken.CreateTime.Add(time.Duration(interval) * time.Second)
		if time.Now().After(limitLifeTime) {
			deleteKeys = append(deleteKeys, key.(string))
		}
		return true
	})

	for _, key := range deleteKeys {
		sessions.Delete(key)
	}
}

func CheckToken(token string) bool {
	_, ok := sessions.Load(token)
	return ok
}

func (user *UserModel) HasAccessToUrl(token string, url string) bool {
	if user.isCommonUrl(url) {
		return true
	}

	value, ok := sessions.Load(token)
	if !ok {
		return false
	}

	userToken, ok := value.(UserToken)
	if !ok {
		return false
	}
	isAdminUser := userToken.UserInfo.IsAdmin

	return isAdminUser
}

func (user *UserModel) isAdminUser(emailPrefix string) bool {
	isAdminUser := false
	adminUsers := user.Ctx.GetConfig().AdminUsers()
	for _, name := range adminUsers {
		if name == emailPrefix {
			isAdminUser = true
			break
		}
	}
	return isAdminUser
}

func (user *UserModel) isCommonUrl(url string) bool {
	isCommonUrl := false
	commonUrls := user.Ctx.GetConfig().CommonUrls()
	for _, item := range commonUrls {
		if item == url {
			isCommonUrl = true
			break
		}
	}
	return isCommonUrl
}
