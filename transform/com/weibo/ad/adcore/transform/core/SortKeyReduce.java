package com.weibo.ad.adcore.transform.core;

import lombok.extern.slf4j.Slf4j;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.mapreduce.Reducer;

import java.io.IOException;

@Slf4j
public class SortKeyReduce extends Reducer<BytesWritable, BytesWritable, BytesWritable, BytesWritable> {

    @Override
    public void setup(Context context) {
    }

    @Override
    public void reduce(BytesWritable key, Iterable<BytesWritable> values, Context context) throws IOException, InterruptedException {
        BytesWritable val = null;
        for (BytesWritable value : values) {
            val = LaserKVEncoder.getValue(value);
        }
        context.write(key, val);
    }
}
