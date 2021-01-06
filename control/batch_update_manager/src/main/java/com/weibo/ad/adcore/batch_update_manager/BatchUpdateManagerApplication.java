package com.weibo.ad.adcore.batch_update_manager;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.scheduling.annotation.EnableScheduling;


@SpringBootApplication
@EnableScheduling
public class BatchUpdateManagerApplication {
    public static void main(String[] args) {
        SpringApplication.run(BatchUpdateManagerApplication.class, args);
    }

}
