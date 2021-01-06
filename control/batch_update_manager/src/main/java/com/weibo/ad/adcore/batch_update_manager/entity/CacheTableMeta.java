package com.weibo.ad.adcore.batch_update_manager.entity;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class CacheTableMeta {

    private String partitionFile;
    private String md5Num;

}
