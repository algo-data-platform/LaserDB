package com.weibo.ad.adcore.transform.job;

import com.weibo.ad.adcore.transform.core.LaserKeyValue;

import java.util.ArrayList;
import java.util.List;

public class KVSeparatorSupportColumnsKey implements KVSeparator {
    @Override
    public LaserKeyValue split(Object data, String delimiter) throws ParseKVException {
        // pk,col1,col2...,val
        String[] arr = data.toString().split(delimiter);
        if (arr.length == 0 || "".equals(arr[0].trim())) {
            throw new ParseKVException("invalid data");
        }
        String pk = arr[0];
        String val = "";
        List<String> columnKeys = new ArrayList<>();
        for (int i = 1; i < arr.length; i++) {
            if (i == arr.length - 1) {
                val = arr[i];
            } else {
                columnKeys.add(arr[i]);
            }
        }
        return LaserKeyValue.builder().primaryKey(pk).columnKeys(columnKeys).value(val).build();
    }
}
