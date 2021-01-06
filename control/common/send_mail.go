package common

import (
	"bytes"
	"crypto/tls"
	"encoding/base64"
	"errors"
	"fmt"
	"net"
	"net/smtp"
)

type LoginAuth struct {
	username, password string
}

func NewLoginAuth(username, password string) smtp.Auth {
	return &LoginAuth{username, password}
}

func (a *LoginAuth) Start(server *smtp.ServerInfo) (string, []byte, error) {
	return "LOGIN", []byte{}, nil
}

func (a *LoginAuth) Next(fromServer []byte, more bool) ([]byte, error) {
	if more {
		switch string(fromServer) {
		case "Username:":
			return []byte(a.username), nil
		case "Password:":
			return []byte(a.password), nil
		default:
			return nil, errors.New("Unknown fromServer")
		}
	}
	return nil, nil
}

func SendMail(addr string, auth smtp.Auth, from string, to []string, subject string, body bytes.Buffer) error {
	c, err := smtp.Dial(addr)
	host, _, _ := net.SplitHostPort(addr)
	if err != nil {
		fmt.Println("call dial err: %v", err)
		return err
	}
	defer c.Close()

	if ok, _ := c.Extension("STARTTLS"); ok {
		config := &tls.Config{ServerName: host, InsecureSkipVerify: true}
		if err = c.StartTLS(config); err != nil {
			fmt.Println("call start tls, err: %v", err)
			return err
		}
	}

	if auth != nil {
		if ok, _ := c.Extension("AUTH"); ok {
			if err = c.Auth(auth); err != nil {
				fmt.Println("check auth with err: %v", err)
				return err
			}
		}
	}

	if err = c.Mail(from); err != nil {
		return err
	}

	var errStr_invalid_mail string
	for _, addr := range to {
		if err := c.Rcpt(addr); err != nil {
			errStr_invalid_mail += fmt.Sprintf("%v; ", err)
		}
	}
	w, err := c.Data()
	if err != nil {
		return err
	}
	header := make(map[string]string)
	header["Subject"] = subject
	header["MIME-Version"] = "1.0"
	header["Content-Type"] = "text/html; charset=\"utf-8\""
	header["Content-Transfer-Encoding"] = "base64"
	message := ""
	for k, v := range header {
		message += fmt.Sprintf("%s: %s\r\n", k, v)
	}
	message += "\r\n" + base64.StdEncoding.EncodeToString(body.Bytes())
	_, err = w.Write([]byte(message))
	if err != nil {
		return err
	}
	err = w.Close()
	if err != nil {
		return err
	}
	if errStr_invalid_mail != "" {
		return fmt.Errorf(errStr_invalid_mail)
	}
	return c.Quit()
}
