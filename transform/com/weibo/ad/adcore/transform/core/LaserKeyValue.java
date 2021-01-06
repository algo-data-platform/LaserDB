package com.weibo.ad.adcore.transform.core;

import lombok.Builder;
import lombok.Data;

import java.util.List;

@Data
@Builder
public class LaserKeyValue {
    private String primaryKey;
    private List<String> columnKeys;
    private String value;
}
