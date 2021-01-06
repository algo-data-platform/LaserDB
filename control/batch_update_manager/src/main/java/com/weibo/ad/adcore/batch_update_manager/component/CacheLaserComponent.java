package com.weibo.ad.adcore.batch_update_manager.component;

import com.weibo.ad.adcore.batch_update_manager.entity.CacheTable;
import com.weibo.ad.adcore.transform.core.CityHash;
import com.weibo.ad.adcore.batch_update_manager.entity.CacheTableVersion;

import lombok.extern.slf4j.Slf4j;

import org.apache.hadoop.fs.FileStatus;
import org.springframework.beans.BeanUtils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import java.time.LocalDateTime;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

@Component
@Slf4j
public class CacheLaserComponent {

    @Autowired
    private HdfsComponent hdfsComponent;

    private Map<Long, CacheTable> cacheLaserMap;

    @Value("${hdfs.basePath}/${hdfs.env}/${hdfs.buildType}")
    private String baseOuputPath;

    private final static String DEFAULTPARTITION = "0";
    private final static String METADATA = "METADATA";
    private final static String VERSION = "VERSION";
    private final static String SEPARATOR = "/";

    public Map<Long, CacheTable> getLaserData() {
        Map<Long, CacheTable> laserCacheTable = new HashMap<>();
        for (Long key : cacheLaserMap.keySet()) {
            String databaseName = cacheLaserMap.get(key).getDatabaseName();
            String tableName = cacheLaserMap.get(key).getTableName();
            CacheTable cacheTable = new CacheTable(databaseName, tableName);
            BeanUtils.copyProperties(cacheLaserMap.get(key), cacheTable);
            laserCacheTable.put(key, cacheTable);
        }
        return laserCacheTable;
    }

    public CacheTable getVersion(String databaseName, String tableName) {
        Long hashKey = getDatabaseTableHash(databaseName, tableName);
        if (!cacheLaserMap.containsKey(hashKey)) {
            return null;
        }
        return cacheLaserMap.get(hashKey);
    }

    public void updateClearData(String databaseName, String tableName, Integer partitionNum, String currentVersion) {
        Long hashKey = getDatabaseTableHash(databaseName, tableName);

        CacheTableVersion version = new CacheTableVersion();
        version.setVersionName(currentVersion);
        version.setVersionTime(LocalDateTime.now());

        if (!cacheLaserMap.containsKey(hashKey)) {
            CacheTable cacheTable = new CacheTable(databaseName, tableName);
            cacheTable.setPartitionNum(partitionNum);

            LinkedList<CacheTableVersion> versionsInfo = new LinkedList<>();
            versionsInfo.add(version);

            cacheTable.setVersionInfo(versionsInfo);
            cacheTable.setCurrentVersion(currentVersion);
            cacheTable.setCurrentVersionTime(LocalDateTime.now());

            cacheLaserMap.put(hashKey, cacheTable);
        } else {
            LinkedList<CacheTableVersion> cacheTableVersions = cacheLaserMap.get(hashKey).getVersionInfo();
            cacheTableVersions.addFirst(version);

            cacheLaserMap.get(hashKey).setVersionInfo(cacheTableVersions);
            cacheLaserMap.get(hashKey).setCurrentVersion(currentVersion);
            cacheLaserMap.get(hashKey).setCurrentVersionTime(LocalDateTime.now());
        }
    }

    public void updateRollbackData(String databaseName, String tableName, String currentVersion) {
        Long hashKey = getDatabaseTableHash(databaseName, tableName);

        if (!cacheLaserMap.containsKey(hashKey)) {
            return;
        }
        cacheLaserMap.get(hashKey).setCurrentVersion(currentVersion);
        cacheLaserMap.get(hashKey).setCurrentVersionTime(LocalDateTime.now());
    }

    @PostConstruct
    private void initCacheLaserComponent() {
        cacheLaserMap = new ConcurrentHashMap<Long, CacheTable>();

        monitorTask();
    }

    @Scheduled(cron = "${laser.monitorTaskCron}")
    public void monitorTask() {
        log.debug("monitorTask start");
        try {
            monitorLaser();
        } catch (Exception e) {
            log.error("monitorTask error:", e);
        }
    }

    //监控laser引擎变化
    private void monitorLaser() throws Exception {
        FileStatus[] dbInfo = hdfsComponent.listFile(getBaseOutputPath());
        if (dbInfo.length == 0) {
            return;
        }

        String tempDatabase;
        for (FileStatus fileStatus : dbInfo) {
            tempDatabase = fileStatus.getPath().getName();
            monitorTable(tempDatabase);
        }
    }

    //监控laser表，表详细的变化【分区数，版本列表，META信息，当前版本】
    private void monitorTable(String databaseName) throws Exception {
        FileStatus[] tableInfo = hdfsComponent.listFile(getDatabasePath(databaseName));
        if (tableInfo.length == 0) {
            return;
        }

        String tempTable;
        for (FileStatus fileStatus : tableInfo) {
            tempTable = fileStatus.getPath().getName();
            CacheTable cacheTable = getCacheTable(databaseName, tempTable);

            //当前版本号
            List<String> versionList = hdfsComponent.readFile(getVersionPath(databaseName, tempTable));
            //当前版本文件信息
            FileStatus versionFileStatus = hdfsComponent.getFileStatus(getVersionPath(databaseName, tempTable));

            if (cacheTable.haveUpdateCurrentVersion(versionList, versionFileStatus)) {
                //版本列表
                FileStatus[] tableVersionInfo = hdfsComponent.listFile(getVersionListPath(databaseName, tempTable));
                cacheTable.updateTableVersion(tableVersionInfo);
                //META信息
                List<String> metaArr = hdfsComponent.readFile(getMetaPath(databaseName, tempTable));
                cacheTable.updateTableMeta(metaArr);
            }

        }
    }

    private CacheTable getCacheTable(String databaseName, String tableName) throws Exception {
        CacheTable cacheTable;
        Long hashKey = getDatabaseTableHash(databaseName, tableName);
        if (!cacheLaserMap.containsKey(hashKey)) {
            cacheTable = initCacheTable(hashKey, databaseName, tableName);
        } else {
            cacheTable = cacheLaserMap.get(hashKey);
        }
        return cacheTable;
    }

    private Long getDatabaseTableHash(String databaseName, String tableName) {
        Long hash = CityHash.cityHash64WithSeed(databaseName.getBytes(), 0, databaseName.length(), 0);
        hash = CityHash.cityHash64WithSeed(tableName.getBytes(), 0, tableName.length(), hash);
        return hash;
    }

    private CacheTable initCacheTable(Long key, String databaseName, String tableName) throws Exception {
        //分区信息
        FileStatus[] tableDetailInfo = hdfsComponent.listFile(getTablePath(databaseName, tableName));
        CacheTable cacheTable = new CacheTable(databaseName, tableName, tableDetailInfo);
        cacheLaserMap.put(key, cacheTable);
        return cacheTable;
    }


    private String getBaseOutputPath() {
        return baseOuputPath;
    }

    private String getDatabasePath(String databaseName) {
        return getBaseOutputPath() + SEPARATOR + databaseName;
    }

    private String getTablePath(String databaseName, String tableName) {
        return getDatabasePath(databaseName) + SEPARATOR + tableName;
    }

    private String getVersionListPath(String databaseName, String tableName) {
        return getTablePath(databaseName, tableName) + SEPARATOR + DEFAULTPARTITION;
    }

    private String getMetaPath(String databaseName, String tableName) {
        return getTablePath(databaseName, tableName) + SEPARATOR + METADATA;
    }

    private String getVersionPath(String databaseName, String tableName) {
        return getTablePath(databaseName, tableName) + SEPARATOR + VERSION;
    }

}
