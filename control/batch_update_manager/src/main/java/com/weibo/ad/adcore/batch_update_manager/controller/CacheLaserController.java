package com.weibo.ad.adcore.batch_update_manager.controller;

import com.weibo.ad.adcore.batch_update_manager.entity.CacheTable;
import com.weibo.ad.adcore.batch_update_manager.entity.CacheTableVersion;
import com.weibo.ad.adcore.batch_update_manager.entity.ResponseTableVersion;
import com.weibo.ad.adcore.batch_update_manager.util.CommonUtil;
import com.weibo.ad.adcore.batch_update_manager.entity.ResponseVersionChange;
import com.weibo.ad.adcore.batch_update_manager.service.CacheLaserService;
import com.weibo.ad.adcore.batch_update_manager.util.ResultJson;

import lombok.extern.slf4j.Slf4j;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

@RestController
@Slf4j
public class CacheLaserController {

    @Autowired
    private CacheLaserService cacheLaserService;

    @GetMapping("/version_change/show")
    public ResultJson show() {
        Map<Long, CacheTable> laserData = cacheLaserService.getLaserData();

        List responseLaser = new ArrayList<>();
        for (Long key : laserData.keySet()) {
            ResponseVersionChange responseVersionChange = new ResponseVersionChange();
            responseVersionChange.setDatabaseName(laserData.get(key).getDatabaseName());
            responseVersionChange.setTableName(laserData.get(key).getTableName());
            if (laserData.get(key).getCurrentVersionTime() == null) {
                continue;
            }
            responseVersionChange.setCurrentVersion(laserData.get(key).getCurrentVersion());
            responseVersionChange.setCurrentVersionTime(CommonUtil.getDateTimeString(laserData.get(key).getCurrentVersionTime()));

            if (laserData.get(key).getVersionInfo() == null) {
                continue;
            }
            List<ResponseTableVersion> responseTableVersionList = handleTableVersion(laserData.get(key));
            responseVersionChange.setVersionInfo(responseTableVersionList);

            responseLaser.add(responseVersionChange);
        }

        return ResultJson.data(responseLaser);
    }

    @GetMapping("/version_change/clear")
    public ResultJson clear(@RequestParam(value = "database_name") String databaseName,
                            @RequestParam(value = "table_name") String tableName,
                            @RequestParam(value = "partition_num") Integer partitionNum) {

        if (databaseName.trim().equals("") || tableName.trim().equals("") || partitionNum == 0) {
            return ResultJson.error("request database_name,table_name,partition_num param error");
        }

        try {
            String currentVersion = cacheLaserService.clearLaserTable(databaseName, tableName, partitionNum);
            return ResultJson.data(currentVersion);
        } catch (Exception e) {
            log.error("CacheLaserController clear error", e);
            return ResultJson.error("CacheLaserController clear error: " + e.toString());
        }
    }

    @GetMapping("/version_change/version")
    public ResultJson version(@RequestParam(value = "database_name") String databaseName,
                              @RequestParam(value = "table_name") String tableName) {
        if (databaseName.trim().equals("") || tableName.trim().equals("")) {
            return ResultJson.error("request database_name,table_name param error");
        }

        CacheTable cacheTable = cacheLaserService.getVersion(databaseName, tableName);

        if (cacheTable == null) {
            return ResultJson.error("cacheTable data error");
        }

        if (cacheTable.getVersionInfo() == null) {
            return ResultJson.error("version data error");
        }
        List<ResponseTableVersion> responseTableVersionList = handleTableVersion(cacheTable);

        return ResultJson.data(responseTableVersionList);
    }

    @GetMapping("/version_change/rollback")
    public ResultJson rollback(@RequestParam(value = "database_name") String databaseName,
                               @RequestParam(value = "table_name") String tableName,
                               @RequestParam(value = "partition_num") Integer partitionNum,
                               @RequestParam(value = "version") String version) {

        if (databaseName.trim().equals("") || tableName.trim().equals("")
                || partitionNum == 0 || version.trim().equals("")) {

            return ResultJson.error("request database_name,table_name,partition_num,version param error");
        }

        try {
            cacheLaserService.rollbackLaserTable(databaseName, tableName, partitionNum, version);
            return ResultJson.data(version);
        } catch (Exception e) {
            log.error("CacheLaserController clear error", e);
            return ResultJson.error(100, "CacheLaserController clear error: " + e.toString());
        }
    }

    private List<ResponseTableVersion> handleTableVersion(CacheTable cacheTable) {
        List<ResponseTableVersion> responseTableVersionList = new ArrayList<>();

        for (CacheTableVersion version : cacheTable.getVersionInfo()) {
            ResponseTableVersion tableVersion = new ResponseTableVersion();
            tableVersion.setVersionName(version.getVersionName());
            tableVersion.setVersionTime(CommonUtil.getDateTimeString(version.getVersionTime()));
            responseTableVersionList.add(tableVersion);
        }
        return responseTableVersionList;
    }

}
