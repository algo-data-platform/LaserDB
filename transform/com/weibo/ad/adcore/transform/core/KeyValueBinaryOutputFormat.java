package com.weibo.ad.adcore.transform.core;

import lombok.extern.slf4j.Slf4j;
import org.apache.commons.codec.binary.Hex;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.io.NullWritable;
import org.apache.hadoop.mapred.FileAlreadyExistsException;
import org.apache.hadoop.mapreduce.JobContext;
import org.apache.hadoop.mapreduce.RecordWriter;
import org.apache.hadoop.mapreduce.TaskAttemptContext;
import org.apache.hadoop.mapreduce.TaskID;
import org.apache.hadoop.mapreduce.lib.output.FileOutputCommitter;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;

import java.io.DataOutputStream;
import java.io.IOException;

public class KeyValueBinaryOutputFormat<K, V> extends FileOutputFormat<K, V> {

    public static String BUILD_VERSION_PROPERTY = "buildVersion";

    public KeyValueBinaryOutputFormat() {
    }

    public RecordWriter<K, V> getRecordWriter(TaskAttemptContext context) throws IOException, InterruptedException {
        Configuration conf = context.getConfiguration();
        TaskID taskId = context.getTaskAttemptID().getTaskID();
        int partition = taskId.getId();

        FileOutputCommitter committer = (FileOutputCommitter) this.getOutputCommitter(context);

        StringBuilder result = new StringBuilder();
        result.append(partition);
        result.append('/');
        String filename = conf.get(BUILD_VERSION_PROPERTY);
        result.append(filename);

        Path file = new Path(committer.getWorkPath(), result.toString());
        FSDataOutputStream fileOut = file.getFileSystem(conf).create(file, true);
        return new KeyValueBinaryRecordWriter<>(fileOut);
    }

    @Override
    public void checkOutputSpecs(JobContext job) throws IOException {
        try {
            super.checkOutputSpecs(job);
        } catch (FileAlreadyExistsException ignored) {
            // Suffocate the exception
        }
    }

    @Slf4j
    protected static class KeyValueBinaryRecordWriter<K, V> extends RecordWriter<K, V> {

        private DataOutputStream out;

        public KeyValueBinaryRecordWriter(DataOutputStream out) {
            this.out = out;
        }

        public synchronized void write(K key, V value) throws IOException {
            boolean nullKey = key == null || key instanceof NullWritable;
            boolean nullValue = value == null || value instanceof NullWritable;
            if (nullKey && nullValue) {
                return;
            }

            if (!(key instanceof BytesWritable) || !(value instanceof BytesWritable)) {
                return;
            }
            // | kv_length | key_length | key | value_length | value |
            BytesWritable keyText = (BytesWritable) key;
            BytesWritable valueText = (BytesWritable) value;
            out.writeInt(keyText.getLength() + valueText.getLength() + 8);
            out.writeInt(keyText.getLength());
            out.write(keyText.copyBytes(), 0, keyText.getLength());
            out.writeInt(valueText.getLength());
            out.write(valueText.copyBytes(), 0, valueText.getLength());
            log.debug("{},{},{},{},{}", keyText.getLength() + valueText.getLength() + 8, keyText.getLength(),
                    Hex.encodeHexString(keyText.copyBytes()), valueText.getLength(),
                    Hex.encodeHexString(valueText.copyBytes()));
        }

        public synchronized void close(TaskAttemptContext context) throws IOException {
            out.close();
        }
    }
}
