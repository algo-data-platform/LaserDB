package com.weibo.ad.adcore.batch_update_manager.service;

import com.weibo.ad.adcore.transform.core.BaseMetaData;
import org.apache.hadoop.conf.Configuration;

public class VersionMetaData extends BaseMetaData {

    public VersionMetaData(String basePath, String env, String databaseName, String tableName, Integer partitionNumber, Configuration configuration) {
        super(basePath, env, databaseName, tableName, partitionNumber, configuration);
    }

    public void setCurrentVersion(String version) {
        this.version = version;
    }
}
