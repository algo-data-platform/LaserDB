package com.weibo.ad.adcore.transform.core;

import lombok.extern.slf4j.Slf4j;
import org.apache.hadoop.io.BytesWritable;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;

@Slf4j
public class LaserKVEncoder {

    public static BytesWritable getValue(BytesWritable value) {
        BytesWritable valText = new BytesWritable();
        byte[] bytes = value.copyBytes();
        byte[] valueBytes = new byte[bytes.length - 4];
        System.arraycopy(bytes, 4, valueBytes, 0, bytes.length - 4);
        valText.set(valueBytes, 0, valueBytes.length);
        return valText;
    }

    public static BytesWritable getPackValue(Integer partitionId, byte[] value) {
        int len = 4 + value.length;
        ByteBuffer buffer = ByteBuffer.allocate(len);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        buffer.putInt(partitionId);
        buffer.put(value);
        return new BytesWritable(buffer.array());
    }

    public static BytesWritable getLaserKey(String key, List<String> columnKeys) {
        if (null == columnKeys) {
            columnKeys = new ArrayList<>();
        }
        // type size + pk vector size + pk0 length + pk0 bytes + cols vector size + col0 length + col0 bytes + ...
        int len = 1 + 4 + 4 + key.getBytes().length + 4 + 4 * columnKeys.size() + columnKeys.stream().mapToInt(s -> s.getBytes().length).sum();
        ByteBuffer buffer = ByteBuffer.allocate(len);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        byte type = 1;
        buffer.put(type);
        // only one pk, so the pk vector size is always 1.
        buffer.putInt(1);
        // pk0 length
        buffer.putInt(key.length());
        // pk0 bytes.
        buffer.put(key.getBytes());
        // cols size
        buffer.putInt(columnKeys.size());
        for (String columnKey : columnKeys) {
            buffer.putInt(columnKey.length());
            buffer.put(columnKey.getBytes());
        }
        return new BytesWritable(buffer.array());
    }
}
