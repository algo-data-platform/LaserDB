package com.weibo.ad.adcore.batch_update_manager.entity;

import com.weibo.ad.adcore.batch_update_manager.component.HdfsComponent;
import com.weibo.ad.adcore.batch_update_manager.util.CommonUtil;

import org.apache.commons.lang.StringUtils;
import org.apache.hadoop.fs.FileStatus;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;

import lombok.Getter;
import lombok.Setter;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.List;

@Getter
@Setter
@Slf4j
public class CacheTable {

    @Autowired
    private HdfsComponent hdfsComponent;

    private String databaseName;
    private String tableName;

    private Integer partitionNum;
    private LinkedList<CacheTableVersion> versionInfo;
    private List<CacheTableMeta> metaInfo;
    private String currentVersion;
    private LocalDateTime currentVersionTime;

    private static final String TABCHARACER = "\t";

    public CacheTable(String databaseName, String tableName) {
        this.databaseName = databaseName;
        this.tableName = tableName;
    }

    public CacheTable(String databaseName, String tableName, FileStatus[] tableDetailInfo) {
        this.databaseName = databaseName;
        this.tableName = tableName;
        initPartionNum(tableDetailInfo);
    }

    private void initPartionNum(FileStatus[] tableDetailInfo) {
        int tempPartitionNum;
        if (tableDetailInfo.length == 0) {
            this.partitionNum = 0;
            return;
        }

        String tempFile;
        int maxPartion = -1;
        int tempNum = 0;
        for (FileStatus fileStatus : tableDetailInfo) {
            tempFile = fileStatus.getPath().getName();
            if (!StringUtils.isNumeric(tempFile)) {
                continue;
            }
            tempNum = Integer.parseInt(tempFile);
            if (tempNum > maxPartion) {
                maxPartion = tempNum;
            }
        }
        tempPartitionNum = maxPartion + 1;
        this.partitionNum = tempPartitionNum;
    }

    public boolean haveUpdateCurrentVersion(List<String> versionList, FileStatus versionStaus) {
        if (versionStaus == null) {
            log.debug("haveUpdateCurrentVersion current version not have update，versionFile have no status");
            return false;
        }

        String version = "";
        if (versionList.size() > 0) {
            version = versionList.get(0);
        }

        if (version.equals(this.currentVersion)) {
            log.debug("haveUpdateCurrentVersion current version not have update");
            return false;
        }

        log.debug("haveUpdateCurrentVersion current version have update");

        this.currentVersion = version;
        LocalDateTime currentVersionTime = CommonUtil.getDateTimeOfTimestamp(versionStaus.getModificationTime());
        this.currentVersionTime = currentVersionTime;
        return true;

    }

    public void updateTableVersion(FileStatus[] tableVersionInfo) {
        LinkedList<CacheTableVersion> cacheTableVersions = new LinkedList<>();

        for (FileStatus fileStatus : tableVersionInfo) {
            CacheTableVersion version = new CacheTableVersion();
            version.setVersionName(fileStatus.getPath().getName());
            version.setVersionTime(CommonUtil.getDateTimeOfTimestamp(fileStatus.getModificationTime()));
            cacheTableVersions.add(version);
        }
        //按时间倒序
        cacheTableVersions.sort(Comparator.comparing(CacheTableVersion::getVersionTime).reversed());
        this.versionInfo = cacheTableVersions;

        log.debug("updateTableVersion table versions have update");
    }

    public void updateTableMeta(List<String> metaArr) {
        List<CacheTableMeta> cacheTableMeta = new ArrayList<CacheTableMeta>();

        for (String metaLine : metaArr) {
            String[] metaLineArr = metaLine.split(TABCHARACER);
            CacheTableMeta metaObj = new CacheTableMeta();
            if (metaLineArr.length >= 2) {
                metaObj.setPartitionFile(metaLineArr[0]);
                metaObj.setMd5Num(metaLineArr[1]);
                cacheTableMeta.add(metaObj);
            }
        }
        this.metaInfo = cacheTableMeta;
    }

}