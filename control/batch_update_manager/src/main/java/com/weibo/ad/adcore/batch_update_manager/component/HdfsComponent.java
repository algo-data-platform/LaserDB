package com.weibo.ad.adcore.batch_update_manager.component;

import org.apache.commons.lang.StringUtils;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.*;
import org.apache.hadoop.io.IOUtils;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;


@Component
public class HdfsComponent {
    private final int bufferSize = 1024 * 1024 * 64;

    private final static String SEPARATOR = "/";

    @Value("${hdfs.hadoopConfigPath}")
    private String hadoopConfigPath;


    /**
     * 获取HDFS配置信息
     *
     * @return
     */
    public Configuration getConfiguration() {
        Configuration configuration = new Configuration();

        String[] configArr = hadoopConfigPath.split(",");
        for (String configPath : configArr) {
            configuration.addResource(new Path(configPath));
        }
        return configuration;
    }

    /**
     * 获取HDFS文件系统对象
     *
     * @return
     * @throws Exception
     */
    public FileSystem getFileSystem() throws Exception {
        FileSystem fileSystem = FileSystem.get(getConfiguration());
        return fileSystem;
    }

    /**
     * 在HDFS创建文件夹
     *
     * @param path
     * @return
     * @throws Exception
     */
    public boolean mkdir(String path) throws Exception {
        if (StringUtils.isEmpty(path)) {
            return false;
        }
        if (existFile(path)) {
            return true;
        }
        FileSystem fs = getFileSystem();
        // 目标路径
        Path srcPath = new Path(path);
        boolean isOk = fs.mkdirs(srcPath);
        fs.close();
        return isOk;
    }

    /**
     * 判断HDFS文件是否存在
     *
     * @param path
     * @return
     * @throws Exception
     */
    public boolean existFile(String path) throws Exception {
        if (StringUtils.isEmpty(path)) {
            return false;
        }
        FileSystem fs = getFileSystem();
        Path srcPath = new Path(path);
        boolean isExists = fs.exists(srcPath);
        return isExists;
    }

    /**
     * 读取HDFS目录信息
     *
     * @param path
     * @return
     * @throws Exception
     */
    public FileStatus[] readPathInfo(String path) throws Exception {
        FileStatus[] statusList;
        if (StringUtils.isEmpty(path)) {
            statusList = new FileStatus[0];
            return statusList;
        }
        if (!existFile(path)) {
            statusList = new FileStatus[0];
            return statusList;
        }
        FileSystem fs = getFileSystem();
        // 目标路径
        Path newPath = new Path(path);
        statusList = fs.listStatus(newPath);
        return statusList;
    }

    /**
     * HDFS创建文件
     *
     * @param path
     * @throws Exception
     */
    public void createEmptyFile(String path) throws Exception {
        FileSystem fs = getFileSystem();
        // 上传时默认当前目录，后面自动拼接文件的目录
        Path newPath = new Path(path);
        // 打开一个输出流
        FSDataOutputStream outputStream = fs.create(newPath);
        outputStream.write("".getBytes());
        outputStream.close();
        fs.close();
    }

    /**
     * 读取HDFS文件内容
     *
     * @param path
     * @return
     * @throws Exception
     */
    public List<String> readFile(String path) throws Exception {
        List<String> contentArr = new ArrayList<String>();

        if (StringUtils.isEmpty(path)) {
            return contentArr;
        }
        if (!existFile(path)) {
            return contentArr;
        }
        FileSystem fs = getFileSystem();
        // 目标路径
        Path srcPath = new Path(path);
        FSDataInputStream inputStream = null;
        try {
            inputStream = fs.open(srcPath);
            // 防止中文乱码
            BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
            String lineTxt = "";
            StringBuffer sb = new StringBuffer();

            while ((lineTxt = reader.readLine()) != null) {
                contentArr.add(lineTxt);
            }
            return contentArr;
        } finally {
            inputStream.close();
            fs.close();
        }
    }

    public FileStatus getFileStatus(String filePath) throws Exception {
        FileStatus fileStatus = new FileStatus();
        if (StringUtils.isEmpty(filePath)) {
            return null;
        }
        if (!existFile(filePath)) {
            return null;
        }

        FileSystem fs = getFileSystem();
        Path path = new Path(filePath);
        fileStatus = fs.getFileStatus(path);
        return fileStatus;
    }

    /**
     * 读取HDFS文件列表
     *
     * @param path
     * @return
     * @throws Exception
     */
    public FileStatus[] listFile(String path) throws Exception {
        FileStatus[] listStatus;

        if (StringUtils.isEmpty(path)) {
            listStatus = new FileStatus[0];
            return listStatus;
        }
        if (!existFile(path)) {
            listStatus = new FileStatus[0];
            return listStatus;
        }

        FileSystem fs = getFileSystem();
        listStatus = fs.listStatus(new Path(path));

        fs.close();
        return listStatus;
    }


    /**
     * HDFS重命名文件
     *
     * @param oldName
     * @param newName
     * @return
     * @throws Exception
     */
    public boolean renameFile(String oldName, String newName) throws Exception {
        if (StringUtils.isEmpty(oldName) || StringUtils.isEmpty(newName)) {
            return false;
        }
        FileSystem fs = getFileSystem();
        // 原文件目标路径
        Path oldPath = new Path(oldName);
        // 重命名目标路径
        Path newPath = new Path(newName);
        boolean isOk = fs.rename(oldPath, newPath);
        fs.close();
        return isOk;
    }

    /**
     * 删除HDFS文件
     *
     * @param path
     * @return
     * @throws Exception
     */
    public boolean deleteFile(String path) throws Exception {
        if (StringUtils.isEmpty(path)) {
            return false;
        }
        if (!existFile(path)) {
            return false;
        }
        FileSystem fs = getFileSystem();
        Path srcPath = new Path(path);
        boolean isOk = fs.deleteOnExit(srcPath);
        fs.close();
        return isOk;
    }

    /**
     * 上传HDFS文件
     *
     * @param path
     * @param uploadPath
     * @throws Exception
     */
    public void uploadFile(String path, String uploadPath) throws Exception {
        if (StringUtils.isEmpty(path) || StringUtils.isEmpty(uploadPath)) {
            return;
        }
        FileSystem fs = getFileSystem();
        // 上传路径
        Path clientPath = new Path(path);
        // 目标路径
        Path serverPath = new Path(uploadPath);

        // 调用文件系统的文件复制方法，第一个参数是否删除原文件true为删除，默认为false
        fs.copyFromLocalFile(false, clientPath, serverPath);
        fs.close();
    }

    /**
     * 下载HDFS文件
     *
     * @param path
     * @param downloadPath
     * @throws Exception
     */
    public void downloadFile(String path, String downloadPath) throws Exception {
        if (StringUtils.isEmpty(path) || StringUtils.isEmpty(downloadPath)) {
            return;
        }
        FileSystem fs = getFileSystem();
        // 上传路径
        Path clientPath = new Path(path);
        // 目标路径
        Path serverPath = new Path(downloadPath);

        // 调用文件系统的文件复制方法，第一个参数是否删除原文件true为删除，默认为false
        fs.copyToLocalFile(false, clientPath, serverPath);
        fs.close();
    }

    /**
     * HDFS文件复制
     *
     * @param sourcePath
     * @param targetPath
     * @throws Exception
     */
    public void copyFile(String sourcePath, String targetPath) throws Exception {
        if (StringUtils.isEmpty(sourcePath) || StringUtils.isEmpty(targetPath)) {
            return;
        }
        FileSystem fs = getFileSystem();
        // 原始文件路径
        Path oldPath = new Path(sourcePath);
        // 目标路径
        Path newPath = new Path(targetPath);

        FSDataInputStream inputStream = null;
        FSDataOutputStream outputStream = null;
        try {
            inputStream = fs.open(oldPath);
            outputStream = fs.create(newPath);

            IOUtils.copyBytes(inputStream, outputStream, bufferSize, false);
        } finally {
            inputStream.close();
            outputStream.close();
            fs.close();
        }
    }


    /**
     * 获取某个文件在HDFS的集群位置
     *
     * @param path
     * @return
     * @throws Exception
     */
    public BlockLocation[] getFileBlockLocations(String path) throws Exception {
        BlockLocation[] blockLocation;

        if (StringUtils.isEmpty(path)) {
            blockLocation = new BlockLocation[0];
            return blockLocation;
        }
        if (!existFile(path)) {
            blockLocation = new BlockLocation[0];
            return blockLocation;
        }
        FileSystem fs = getFileSystem();
        // 目标路径
        Path srcPath = new Path(path);
        FileStatus fileStatus = fs.getFileStatus(srcPath);
        blockLocation = fs.getFileBlockLocations(fileStatus, 0, fileStatus.getLen());
        if (blockLocation == null) {
            blockLocation = new BlockLocation[0];
        }
        return blockLocation;
    }

}



