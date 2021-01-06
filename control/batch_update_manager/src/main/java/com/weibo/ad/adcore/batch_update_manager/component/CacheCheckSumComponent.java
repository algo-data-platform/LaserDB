package com.weibo.ad.adcore.batch_update_manager.component;

import lombok.extern.slf4j.Slf4j;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

@Component
@Slf4j
public class CacheCheckSumComponent {

    @Autowired
    private HdfsComponent hdfsComponent;

    private Map<String, String> cacheCheckSumInfo;

    public String getCheckSumInfo(String path) throws Exception {
        if (cacheCheckSumInfo.containsKey(path)) {
            return cacheCheckSumInfo.get(path);
        }
        String value = getSpecCheckSumInfo(path);
        cacheCheckSumInfo.put(path, value);
        return value;
    }

    @PostConstruct
    private void initCacheCheckSumComponent() {
        cacheCheckSumInfo =  new ConcurrentHashMap<String, String>();

        monitorTask();
    }

    @Scheduled(cron = "${checksum.monitorTaskCron}")
    public void monitorTask() {
        log.debug("monitorTask start");
        try {
            monitorCheckSum();
        } catch (Exception e) {
            log.error("monitorTask error:", e);
        }
    }

     //监控并更新CheckSumInfo变化
     private void monitorCheckSum() throws Exception {
        if (cacheCheckSumInfo.isEmpty()) {
            return;
        }
        for (String pathKey : cacheCheckSumInfo.keySet()) { 
            String value = getSpecCheckSumInfo(pathKey);
            cacheCheckSumInfo.put(pathKey, value);
        } 
    }

     // 获取指定path的checksum信息
     private String getSpecCheckSumInfo(String path) throws Exception {
        List<String> CheckSumInfoList = hdfsComponent.readFile(path);
        String value = "";
        for (String str : CheckSumInfoList) {
            value += str + "\n";
        }
        return value;
    }
}
