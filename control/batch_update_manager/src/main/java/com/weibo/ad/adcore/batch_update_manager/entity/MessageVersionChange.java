package com.weibo.ad.adcore.batch_update_manager.entity;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class MessageVersionChange {
    private String databaseName;
    private String tableName;
    private String versionName;
    private String versionTime;
}