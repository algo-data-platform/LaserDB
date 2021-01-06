package com.weibo.ad.adcore.transform.job;

import com.weibo.ad.adcore.transform.core.LaserKeyValue;

@FunctionalInterface
public interface KVSeparator {
    LaserKeyValue split(Object data, String delimiter) throws ParseKVException;
}
