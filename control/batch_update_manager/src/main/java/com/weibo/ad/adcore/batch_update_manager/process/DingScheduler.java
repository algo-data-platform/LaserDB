package com.weibo.ad.adcore.batch_update_manager.process;

import com.weibo.ad.adcore.batch_update_manager.entity.DingResult;
import com.weibo.ad.adcore.batch_update_manager.entity.MessageVersionChange;
import com.weibo.ad.adcore.batch_update_manager.service.CacheLaserService;
import com.weibo.ad.adcore.batch_update_manager.util.DingUtil;

import lombok.extern.slf4j.Slf4j;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import java.util.List;

@Component
@EnableScheduling
@Slf4j
public class DingScheduler {

    private static final String DING_TITLE = "laser表每日版本变更列表";
    private static final String DING_CONTENT = "laser表每日版本变更列表";
    private static final String LINE_BREAK = "\n";

    @Autowired
    private CacheLaserService cacheLaserService;

    @Autowired
    private DingUtil dingUtil;

    //0:当天变更记录
    private static final int CURRENT_FILTER_DAY = 0;

    @Scheduled(cron = "${laser.dingdingTaskCron}")
    public void sendDingTask() {
        try {
            List<MessageVersionChange> messageVersionChangeList = cacheLaserService.getMessageVersionChange(CURRENT_FILTER_DAY);

            String dingText = DING_CONTENT + LINE_BREAK;
            for (MessageVersionChange messageVersionChange : messageVersionChangeList) {
                dingText = dingText + String.format("- %s-%s: %s %s " + LINE_BREAK,
                        messageVersionChange.getDatabaseName(),
                        messageVersionChange.getTableName(),
                        messageVersionChange.getVersionName(),
                        messageVersionChange.getVersionTime());
            }

            DingResult dingResult = dingUtil.messageMarkdown(dingText, DING_TITLE, new String[0], false);
            if (dingResult.getErrcode() != 0) {
                log.error(String.format("sendDingTask send error | %d:%s", dingResult.getErrcode(), dingResult.getErrmsg()));
            }
            log.info("sendDingTask send success");
        } catch (Exception e) {
            log.error("sendDingTask send error |", e);
        }
    }
}
