package com.weibo.ad.adcore.transform.job;

import com.weibo.ad.adcore.transform.core.Constants;
import com.weibo.ad.adcore.transform.core.LaserKVEncoder;
import com.weibo.ad.adcore.transform.core.LaserKeyValue;
import com.weibo.ad.adcore.transform.core.Partition;
import lombok.extern.slf4j.Slf4j;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.mapreduce.Mapper;
import org.xerial.snappy.Snappy;

import java.io.IOException;

@Slf4j
public class PackKeyMapper extends Mapper<Object, Object, BytesWritable, BytesWritable> {
    private String databaseName = "";
    private String tableName = "";
    private String keyValueDelimiterRegex = "";
    private Integer partitionNumber = 0;
    private String valueCompressMethod = "";
    private String inputFileType = "";

    @Override
    public void setup(Context context) {
        this.databaseName = context.getConfiguration().get(Constants.DATABASE_NAME_PROPERTY);
        this.tableName = context.getConfiguration().get(Constants.TABLE_NAME_PROPERTY);
        this.keyValueDelimiterRegex = context.getConfiguration().get(Constants.DELIMITER_PROPERTY);
        this.partitionNumber = context.getConfiguration().getInt(Constants.PARTITION_NUMBER_PROPERTY, 0);
        this.valueCompressMethod = context.getConfiguration().get(Constants.VALUE_COMPRESS_METHOD);
        this.inputFileType = context.getConfiguration().get(Constants.INPUT_FILE_TYPE);
    }

    @Override
    public void map(Object key, Object data, Context context)
            throws IOException, InterruptedException {
        LaserKeyValue laserKeyValue = LaserKeyValue.builder().build();
        byte[] value;
        if (inputFileType.toLowerCase().equals(Constants.SEQUENCE_FILE)) {
            value = ((BytesWritable) data).copyBytes();
            laserKeyValue.setPrimaryKey(key.toString());
        } else {
            KVSeparator kvSeparator = new KVSeparatorSupportColumnsKey();
            try {
                laserKeyValue = kvSeparator.split(data, keyValueDelimiterRegex);
            } catch (Exception e) {
                log.error(e.getMessage(), e);
                return;
            }
            // 判断压缩方法
            if (valueCompressMethod.toLowerCase().equals("snappy")) {
                value = Snappy.compress(laserKeyValue.getValue().getBytes());
            } else {
                value = laserKeyValue.getValue().getBytes();
            }
        }

        BytesWritable laser_key = LaserKVEncoder.getLaserKey(laserKeyValue.getPrimaryKey(), laserKeyValue.getColumnKeys());
        Integer partitionId = Partition.getPartitionId(databaseName, tableName, laserKeyValue.getPrimaryKey(), partitionNumber);
        context.write(laser_key, LaserKVEncoder.getPackValue(partitionId, value));
        context.getCounter(CounterName.TotalKey).increment(1);
    }
}
