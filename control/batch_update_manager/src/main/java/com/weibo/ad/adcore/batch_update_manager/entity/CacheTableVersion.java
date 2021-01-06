package com.weibo.ad.adcore.batch_update_manager.entity;

import lombok.Getter;
import lombok.Setter;

import java.time.LocalDateTime;

@Getter
@Setter
public class CacheTableVersion {
    private String versionName;
    private LocalDateTime versionTime;
}
