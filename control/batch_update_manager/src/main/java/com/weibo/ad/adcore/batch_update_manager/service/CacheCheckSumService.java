package com.weibo.ad.adcore.batch_update_manager.service;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import com.weibo.ad.adcore.batch_update_manager.component.CacheCheckSumComponent;


@Service
public class CacheCheckSumService {

    @Autowired
    private CacheCheckSumComponent dataComponent;
    

    public String getCheckSumInfo(String path) throws Exception {
        return dataComponent.getCheckSumInfo(path);
    }

}