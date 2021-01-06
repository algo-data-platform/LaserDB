package com.weibo.ad.adcore.transform.core;

import org.apache.hadoop.conf.Configuration;

public class BaseMetaData extends MetaData {

    public BaseMetaData(String basePath, String env, String databaseName, String tableName, Integer partitionNumber, Configuration configuration) {
        super(basePath, env, databaseName, tableName, partitionNumber, configuration);
    }

    public String getOutputParentPath() {
        return getBaseOutputPath();
    }
}
