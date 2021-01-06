package com.weibo.ad.adcore.transform.core;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FileSystem;

import lombok.extern.slf4j.Slf4j;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

@Slf4j
public class DeltaMetaData extends MetaData {

    private final static String DEFAULT_BASE_VERSION = "default";

    private String baseVersion;

    public DeltaMetaData(String basePath, String env, String databaseName, String tableName, Integer partitionNumber, Configuration configuration) {
        super(basePath, env, databaseName, tableName, partitionNumber, configuration);
        this.baseVersion = getBaseVersion();
    }

    public String getOutputParentPath() {
        StringBuilder path = new StringBuilder();
        path.append(basePath);
        path.append('/');
        path.append(env);
        path.append('/');
        path.append(DELTA_PATH);
        path.append('/');
        String tablePath = getTableOutputPath();
        path.append(tablePath);
        path.append('/');
        path.append(baseVersion);

        return path.toString();
    }

    private String getBaseVersion() {
        String version = DEFAULT_BASE_VERSION;
        try {
            FileSystem fs = FileSystem.get(configuration);
            FSDataInputStream fsDataInputStream = fs.open(getBaseVersionPath());
            BufferedReader reader = new BufferedReader(new InputStreamReader(fsDataInputStream));
            String line;
            if ((line = reader.readLine()) != null) {
                version = line;
            }
            reader.close();
            fsDataInputStream.close();
            fs.close();
        } catch (IOException e) {
            log.error(e.getMessage(), e);
        }
        return version;
    }
}
