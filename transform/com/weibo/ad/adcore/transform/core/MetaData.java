package com.weibo.ad.adcore.transform.core;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.permission.FsAction;
import org.apache.hadoop.fs.permission.FsPermission;

import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

abstract public class MetaData {

    private final static String METADATA_PATH = "METADATA";
    private final static String VERSION_PATH = "VERSION";
    protected final static String BASE_PATH = "base";
    protected final static String DELTA_PATH = "delta";

    private static FsPermission permission = new FsPermission(FsAction.READ_WRITE, FsAction.READ_WRITE, FsAction.READ_WRITE);

    protected String basePath;
    protected String env;
    protected String databaseName;
    protected String tableName;
    protected String version;
    protected Integer partitionNumber;

    protected Configuration configuration;

    {
        this.version = generatorVersion();
    }

    public MetaData(String basePath, String env, String databaseName, String tableName, Integer partitionNumber, Configuration configuration) {
        this.basePath = basePath;
        this.env = env;
        this.databaseName = databaseName;
        this.tableName = tableName;
        this.configuration = configuration;
        this.partitionNumber = partitionNumber;
    }

    abstract public String getOutputParentPath();

    protected Path getBaseMetaDataPath() {
        return new Path(getBaseOutputPath(), METADATA_PATH);
    }

    protected Path getBaseVersionPath() {
        return new Path(getBaseOutputPath(), VERSION_PATH);
    }

    protected Path getMetaDataPath() {
        return new Path(getOutputParentPath(), METADATA_PATH);
    }

    protected Path getVersionPath() {
        return new Path(getOutputParentPath(), VERSION_PATH);
    }

    protected String getTableOutputPath() {
        StringBuilder result = new StringBuilder();
        result.append(databaseName);
        result.append('/');
        result.append(tableName);
        return result.toString();
    }

    protected String getBaseOutputPath() {
        StringBuilder path = new StringBuilder();
        path.append(basePath);
        path.append('/');
        path.append(env);
        path.append('/');
        path.append(BASE_PATH);
        path.append('/');
        String tablePath = getTableOutputPath();
        path.append(tablePath);
        return path.toString();
    }

    public String getCurrentVersion() {
        return version;
    }

    public Path getFilePath(Integer partId) {
        StringBuilder childPath = new StringBuilder();
        childPath.append(partId);
        childPath.append('/');
        childPath.append(version);
        return new Path(getOutputParentPath(), childPath.toString());
    }

    protected String generatorVersion() {
        Date d = new Date();
        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMddHHmmss");

        String timestamp = String.valueOf(d.getTime());
        Random versionSeed = new Random();
        String hash = String.valueOf(Math.abs(CityHash.cityHash64WithSeed(timestamp.getBytes(), 0, timestamp.length(), versionSeed.nextLong())));
        StringBuilder result = new StringBuilder();
        result.append(sdf.format(d));
        result.append('_');
        result.append(hash);
        return result.toString();
    }

    public void generatorMetadata() throws IOException {
        FileSystem fs = FileSystem.get(configuration);
        StringBuilder result = new StringBuilder();
        for (Integer partId = 0; partId < partitionNumber; partId++) {
            Path file = getFilePath(partId);
            if (!fs.exists(file)) {
                continue;
            }

            result.append(file.toUri().getPath());
            result.append('\t');
            result.append(fs.getFileChecksum(file).toString());
            result.append('\n');
        }

        if (result.toString().length() != 0) {
            Path metaDataPath = getMetaDataPath();
            FSDataOutputStream outputStream = fs.create(metaDataPath);
            fs.setPermission(metaDataPath, permission);
            outputStream.write(result.toString().getBytes(), 0, result.length());
            outputStream.close();
        }
        fs.close();
    }

    public void generatorVersionFile() throws IOException {
        Path versionPath = getVersionPath();
        FileSystem fs = FileSystem.get(configuration);
        FSDataOutputStream outputStream = fs.create(versionPath);
        fs.setPermission(versionPath, permission);
        outputStream.write(getCurrentVersion().getBytes(), 0, getCurrentVersion().length());
        outputStream.close();
        fs.close();
    }
}
