package com.weibo.ad.adcore.transform.core;

import org.apache.hadoop.io.BytesWritable;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class Partition {
    public static int getPartitionId(BytesWritable value) {
        byte[] partitionIdBytes = new byte[4];
        System.arraycopy(value.getBytes(), 0, partitionIdBytes, 0, 4);

        ByteBuffer partitionIdBuffer = ByteBuffer.allocate(4);
        partitionIdBuffer.order(ByteOrder.LITTLE_ENDIAN);
        partitionIdBuffer.put(partitionIdBytes);
        return partitionIdBuffer.getInt(0);
    }

    public static int getPartitionId(String dbname, String tablename, String primaryKey, int partitionNumber) {
        long hash = CityHash.cityHash64WithSeed(primaryKey.getBytes(), 0, primaryKey.length(), 0);
        hash = CityHash.cityHash64WithSeed(dbname.getBytes(), 0, dbname.length(), hash);
        hash = CityHash.cityHash64WithSeed(tablename.getBytes(), 0, tablename.length(), hash);
        return (int) (Math.abs(hash) % partitionNumber);
    }
}
