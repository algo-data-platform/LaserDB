package com.weibo.ad.adcore.batch_update_manager.service;

import com.weibo.ad.adcore.batch_update_manager.entity.CacheTable;
import com.weibo.ad.adcore.batch_update_manager.entity.CacheTableVersion;
import com.weibo.ad.adcore.batch_update_manager.entity.MessageVersionChange;
import com.weibo.ad.adcore.batch_update_manager.util.CommonUtil;

import org.apache.hadoop.fs.Path;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;
import java.util.*;

import com.weibo.ad.adcore.batch_update_manager.component.HdfsComponent;
import com.weibo.ad.adcore.batch_update_manager.component.CacheLaserComponent;

@Service
public class CacheLaserService {

    @Autowired
    private CacheLaserComponent dataComponent;

    @Autowired
    private HdfsComponent hdfsComponent;

    @Value("${hdfs.basePath}")
    private String basePath;

    @Value("${hdfs.env}")
    private String env;

    public Map<Long, CacheTable> getLaserData() {
        Map<Long, CacheTable> laserData = dataComponent.getLaserData();
        return laserData;
    }

    public List<MessageVersionChange> getMessageVersionChange(Integer filterDay) {
      Map<Long, CacheTable> laserData = getLaserData();

      //筛选符合日期的版本信息
      LocalDateTime filterLocaleDateTime = CommonUtil.getDateTimeOfDay(filterDay);

      LinkedList<MessageVersionChange> messageVersionChangeList = new LinkedList<>();
      for (Long key : laserData.keySet()) {
          if (laserData.get(key).getVersionInfo() == null) {
              continue;
          }
          for (CacheTableVersion tableVersion : laserData.get(key).getVersionInfo()) {
              if (!filterLocaleDateTime.isBefore(tableVersion.getVersionTime())) {
                  continue;
              }
              MessageVersionChange messageVersionChange = new MessageVersionChange();
              messageVersionChange.setDatabaseName(laserData.get(key).getDatabaseName());
              messageVersionChange.setTableName(laserData.get(key).getTableName());
              messageVersionChange.setVersionName(tableVersion.getVersionName());
              messageVersionChange.setVersionTime(CommonUtil.getDateTimeString(tableVersion.getVersionTime()));
              messageVersionChangeList.add(messageVersionChange);
          }
      }
      //按版本变更时间倒序
      messageVersionChangeList.sort(Comparator.comparing(MessageVersionChange::getVersionTime).reversed());
      return messageVersionChangeList;
    }

    public CacheTable getVersion(String databaseName, String tableName) {
        CacheTable cacheTable = dataComponent.getVersion(databaseName, tableName);
        return cacheTable;
    }

    public String clearLaserTable(String databaseName, String tableName, Integer partitionNum) throws Exception {
        VersionMetaData versionData = new VersionMetaData(basePath,
                env, databaseName, tableName, partitionNum, hdfsComponent.getConfiguration());

        for (Integer partitionId = 0; partitionId < partitionNum; partitionId++) {
            Path file = versionData.getFilePath(partitionId);
            hdfsComponent.createEmptyFile(file.toString());
        }
        versionData.generatorMetadata();
        versionData.generatorVersionFile();
        String currentVersion = versionData.getCurrentVersion();

        dataComponent.updateClearData(databaseName, tableName, partitionNum, currentVersion);

        return currentVersion;
    }

    public void rollbackLaserTable(String databaseName, String tableName, Integer partitionNum, String version) throws Exception {
        VersionMetaData versionData = new VersionMetaData(basePath,
                env, databaseName, tableName, partitionNum, hdfsComponent.getConfiguration());

        versionData.setCurrentVersion(version);

        for (Integer partitionId = 0; partitionId < partitionNum; partitionId++) {
            Path file = versionData.getFilePath(partitionId);
            if (!hdfsComponent.existFile(file.toString())) {
                throw new Exception("version data not exist");
            }
        }

        versionData.generatorMetadata();
        versionData.generatorVersionFile();

        dataComponent.updateRollbackData(databaseName, tableName, version);
    }

}