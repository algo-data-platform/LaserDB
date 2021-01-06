package com.weibo.ad.adcore.batch_update_manager.util;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.Date;

public class CommonUtil {

    public static LocalDateTime getDateTimeOfTimestamp(long timestamp) {
        Instant instant = Instant.ofEpochMilli(timestamp);
        ZoneId zone = ZoneId.systemDefault();
        return LocalDateTime.ofInstant(instant, zone);
    }

    public static String getDateTimeString(LocalDateTime dateTime) {
        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        String formattedDateTime = dateTime.format(formatter);
        return formattedDateTime;
    }

    public static LocalDateTime getDateTimeOfDay(Integer filterDay) {
        Date date = new Date(System.currentTimeMillis() - filterDay * 1000 * 60 * 60 * 24);
        DateFormat format = new SimpleDateFormat("yyyy-MM-dd");
        String str = format.format(date) + " 00:00:00";

        DateTimeFormatter fmt = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        LocalDateTime localDateTime = LocalDateTime.parse(str, fmt);
        return localDateTime;
    }
}
