package com.weibo.ad.adcore.batch_update_manager.util;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class ResultJson<T> {
    @JsonProperty("Code")
    private int code;
    @JsonProperty("Message")
    private String message;
    @JsonProperty("Data")
    private T data;

    public static final ResultJson SUCCESS_RESULT = new ResultJson(0, "success");
    public static final int ERROR = 10001;

    public ResultJson() {
    }

    public ResultJson(T data) {
        this.code = ResultJson.SUCCESS_RESULT.getCode();
        this.message = ResultJson.SUCCESS_RESULT.getMessage();
        this.data = data;
    }

    public ResultJson(int code, String msg) {
        this.code = code;
        this.message = msg;
    }

    public ResultJson(int code, String msg, T data) {
        this.code = code;
        this.message = msg;
        this.data = data;
    }

    public ResultJson(ResultJson param, T data) {
        this.code = param.getCode();
        this.message = param.getMessage();
        this.data = data;
    }

    public static <T> ResultJson<T> error(String message) {
        return (ResultJson<T>) new ResultJson(ERROR, message);
    }

    public static <T> ResultJson<T> error(int code, String message) {
        return (ResultJson<T>) new ResultJson(code, message);
    }

    public static <T> ResultJson<T> data(T data) {
        return (ResultJson<T>) new ResultJson(SUCCESS_RESULT, data);
    }

}