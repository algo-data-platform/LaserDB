package com.weibo.ad.adcore.batch_update_manager.controller;

import com.weibo.ad.adcore.batch_update_manager.service.CacheCheckSumService;

import lombok.extern.slf4j.Slf4j;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.ResponseBody;
import org.springframework.web.bind.annotation.RestController;

@RestController
@Slf4j
public class CacheCheckSumController {
    @Autowired
    private CacheCheckSumService cacheCheckSumService;

    @RequestMapping(value = "/checksum", method = RequestMethod.GET)
    @ResponseBody
    public String linkGetParams(@RequestParam(value = "path") String path) {
        try {
            return cacheCheckSumService.getCheckSumInfo(path);
        } catch (Exception e) {
            log.error("getCheckSumInfo error:", e);
        }

        return "error";
    }

}

