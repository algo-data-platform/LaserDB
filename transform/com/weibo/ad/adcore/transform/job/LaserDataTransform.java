package com.weibo.ad.adcore.transform.job;

import com.weibo.ad.adcore.transform.core.*;
import lombok.extern.slf4j.Slf4j;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.SequenceFileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.util.GenericOptionsParser;
import org.apache.log4j.BasicConfigurator;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

@Slf4j
public class LaserDataTransform {
    private static Integer partitionNumber = 0;
    private static String inputFileType = "";
    private static MetaData metaData = null;

    private static void initJob(String configFile, Configuration conf) {
        Properties prop = new Properties();
        InputStream input = null;
        try {
            input = new FileInputStream(configFile);
            prop.load(input);
            String databaseName = prop.getProperty(Constants.DATABASE_NAME_PROPERTY).trim();
            String tableName = prop.getProperty(Constants.TABLE_NAME_PROPERTY).trim();
            String env = prop.getProperty(Constants.ENV_PROPERTY).trim();
            String basePath = prop.getProperty(Constants.BASE_PATH_PROPERTY).trim();
            String buildType = prop.getProperty(Constants.BUILD_TYPE_PROPERTY).trim();
            String compressMethod = prop.getProperty(Constants.VALUE_COMPRESS_METHOD).trim();
            String fileType = prop.getProperty(Constants.INPUT_FILE_TYPE).trim();
            LaserDataTransform.inputFileType = fileType;
            Integer partitionNumber = Integer.parseInt(prop.getProperty(Constants.PARTITION_NUMBER_PROPERTY));
            LaserDataTransform.partitionNumber = partitionNumber;

            conf.set(Constants.DATABASE_NAME_PROPERTY, databaseName);
            conf.set(Constants.TABLE_NAME_PROPERTY, tableName);
            conf.set(Constants.DELIMITER_PROPERTY, prop.getProperty(Constants.DELIMITER_PROPERTY));
            conf.setInt(Constants.PARTITION_NUMBER_PROPERTY, partitionNumber);
            conf.set(Constants.VALUE_COMPRESS_METHOD, compressMethod);
            conf.set(Constants.INPUT_FILE_TYPE, fileType);

            if (buildType.equals(Constants.BUILD_TYPE_BASE)) {
                metaData = new BaseMetaData(basePath, env, databaseName, tableName, partitionNumber, conf);
            } else {
                metaData = new DeltaMetaData(basePath, env, databaseName, tableName, partitionNumber, conf);
            }
            conf.set(Constants.BUILD_VERSION_PROPERTY, metaData.getCurrentVersion());
        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            if (input != null) {
                try {
                    input.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public static void main(String[] args)
            throws IOException, ClassNotFoundException, InterruptedException {
        BasicConfigurator.configure();
        Configuration conf = new Configuration();
        String[] otherArgs = new GenericOptionsParser(conf, args).getRemainingArgs();
        conf.set("fs.permissions.umask-mode", "000");
        initJob(otherArgs[1], conf);
        if (otherArgs.length != 3) {
            System.err.println("Usage: laser_data_transform <in> <out>");
        }

        Job job = Job.getInstance(conf, "LaserDataTransform");
        job.setJarByClass(LaserDataTransform.class);
        job.setMapperClass(PackKeyMapper.class);
        job.setReducerClass(SortKeyReduce.class);
        if (LaserDataTransform.inputFileType.toLowerCase().equals(Constants.SEQUENCE_FILE)) {
            job.setInputFormatClass(SequenceFileInputFormat.class);
        }
        job.setOutputKeyClass(BytesWritable.class);
        job.setOutputValueClass(BytesWritable.class);
        job.setMapOutputKeyClass(BytesWritable.class);
        job.setMapOutputValueClass(BytesWritable.class);
        job.setPartitionerClass(PartitionHash.class);
        // reduce task 个数必须和 partition number 一致
        job.setNumReduceTasks(LaserDataTransform.partitionNumber);
        job.setOutputFormatClass(KeyValueBinaryOutputFormat.class);
        FileInputFormat.addInputPath(job, new Path(otherArgs[2]));
        FileOutputFormat.setOutputPath(job, new Path(metaData.getOutputParentPath()));
        if (job.waitForCompletion(true)) {
            log.info("Total key:" + job.getCounters().findCounter(CounterName.TotalKey).getValue());
            try {
                metaData.generatorMetadata();
                metaData.generatorVersionFile();
            } catch (IOException e) {
                log.info(e.getMessage());
            }
            System.exit(0);
        } else {
            System.exit(1);
        }
    }
}
