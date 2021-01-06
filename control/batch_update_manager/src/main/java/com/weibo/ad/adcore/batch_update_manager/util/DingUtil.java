package com.weibo.ad.adcore.batch_update_manager.util;

import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;
import com.weibo.ad.adcore.batch_update_manager.entity.DingResult;
import org.apache.commons.codec.binary.Base64;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;
import okhttp3.*;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import java.net.URLEncoder;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

@Component
public class DingUtil {
    @Value("${dingding.secret}")
    public String dingSecret;

    @Value("${dingding.url}")
    private String dingHost;

    @Value("${dingding.accessToken}")
    private String dingToken;

    @Value("${dingding.enable}")
    private boolean dingEnable;

    private static final int CONNECT_TIMEOUT = 10;
    private static final int WRITE_IMEOUT = 10;
    private static final int READ_TIMEOUT = 20;

    private static final String MESSAGE_TYPE = "markdown";
    private static final String CONTENT_TYPE = "application/json; charset=utf-8";

    public DingResult messageMarkdown(String text, String title, String[] at, boolean isAtAll) throws Exception {
        DingResult dingResult = new DingResult();
        if (!dingEnable) {
            return dingResult;
        }
        HashMap<String, Object> messageMap = new HashMap<>();
        messageMap.put("msgtype", MESSAGE_TYPE);

        HashMap<String, Object> markdownMap = new HashMap<>();
        markdownMap.put("text", text);
        markdownMap.put("title", title);
        messageMap.put("markdown", markdownMap);

        HashMap<String, Object> atMap = new HashMap<>();
        atMap.put("atMobiles", at);
        messageMap.put("at", atMap);

        messageMap.put("isAtAll", isAtAll);

        String MessageResult = postData(messageMap);
        JSONObject jsonObject = JSONObject.parseObject(MessageResult);
        dingResult = JSON.toJavaObject(jsonObject, DingResult.class);
        return dingResult;
    }

    private String postData(Map body) throws Exception {
        //创建OkHttpClient对象
        OkHttpClient okHttpClient = new OkHttpClient.Builder()
                .connectTimeout(CONNECT_TIMEOUT, TimeUnit.SECONDS)
                .writeTimeout(WRITE_IMEOUT, TimeUnit.SECONDS)
                .readTimeout(READ_TIMEOUT, TimeUnit.SECONDS)
                .build();

        //MediaType  设置Content-Type 标头中包含的媒体类型值
        RequestBody requestBody = RequestBody.create(MediaType.parse(CONTENT_TYPE)
                , JSON.toJSONString(body));

        Long timestamp = System.currentTimeMillis();
        String sign = getSign(timestamp);
        String sendApiUrl = String.format("%s?access_token=%s&timestamp=%d&sign=%s", dingHost, dingToken, timestamp, sign);

        Request request = new Request.Builder().url(sendApiUrl)//请求的url
                .post(requestBody)
                .build();

        Response response = okHttpClient.newCall(request).execute();
        String returnString = response.body().string();
        return returnString;
    }

    private String getSign(Long timestamp) throws Exception {
        String stringToSign = String.format("%d\n%s", timestamp, dingSecret);
        Mac mac = Mac.getInstance("HmacSHA256");
        mac.init(new SecretKeySpec(dingSecret.getBytes("UTF-8"), "HmacSHA256"));
        byte[] signData = mac.doFinal(stringToSign.getBytes("UTF-8"));
        String sign = URLEncoder.encode(new String(Base64.encodeBase64(signData)), "UTF-8");
        return sign;
    }

}
