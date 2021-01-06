package com.weibo.ad.adcore.batch_update_manager.entity;

import lombok.Getter;
import lombok.Setter;

import java.util.List;

@Getter
@Setter
public class ResponseVersionChange {
    private String databaseName;
    private String tableName;
    private List<ResponseTableVersion> versionInfo;
    private String currentVersion;
    private String currentVersionTime;
}


