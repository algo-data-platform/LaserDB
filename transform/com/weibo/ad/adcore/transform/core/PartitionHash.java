package com.weibo.ad.adcore.transform.core;

import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.mapreduce.Partitioner;

public class PartitionHash extends Partitioner<BytesWritable, BytesWritable> {
    @Override
    public int getPartition(BytesWritable key, BytesWritable val, int numTasks) {
        return Partition.getPartitionId(val);
    }
}
