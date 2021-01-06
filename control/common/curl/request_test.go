package curl

import "testing"

func TestRequestGet(t *testing.T) {

	url := "http://ip:port/laser/version_change"

	// 链式操作
	req := NewRequest()
	resp, err := req.
		SetUrl(url).
		Get()

	if err != nil {
		t.Log(err)
	} else {
		if resp.IsOk() {
			t.Log(resp.Body)
		} else {
			t.Log(resp.Raw)
		}
	}
}

func TestRequestPost(t *testing.T) {

	url := "http://ip:port/laser/version_change"

	headers := map[string]string{
		"Content-Type": "application/json",
	}

	cookies := map[string]string{
		"userId": "test",
	}

	queries := map[string]string{
		"act": "show",
	}

	postData := map[string]interface{}{
		"name": "test",
	}

	// 链式操作
	req := NewRequest()
	resp, err := req.
		SetUrl(url).
		SetHeaders(headers).
		SetCookies(cookies).
		SetQueries(queries).
		SetPostData(postData).
		Post()

	if err != nil {
		t.Log(err)
	} else {
		if resp.IsOk() {
			t.Log(resp.Body)
		} else {
			t.Log(resp.Raw)
		}
	}
}
