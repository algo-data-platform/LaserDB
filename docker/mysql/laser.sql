-- MySQL dump 10.13  Distrib 8.0.22-13, for Linux (x86_64)
--
-- Host: localhost    Database: laser
-- ------------------------------------------------------
-- Server version	8.0.22-13

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
/*!50717 SELECT COUNT(*) INTO @rocksdb_has_p_s_session_variables FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = 'performance_schema' AND TABLE_NAME = 'session_variables' */;
/*!50717 SET @rocksdb_get_is_supported = IF (@rocksdb_has_p_s_session_variables, 'SELECT COUNT(*) INTO @rocksdb_is_supported FROM performance_schema.session_variables WHERE VARIABLE_NAME=\'rocksdb_bulk_load\'', 'SELECT 0') */;
/*!50717 PREPARE s FROM @rocksdb_get_is_supported */;
/*!50717 EXECUTE s */;
/*!50717 DEALLOCATE PREPARE s */;
/*!50717 SET @rocksdb_enable_bulk_load = IF (@rocksdb_is_supported, 'SET SESSION rocksdb_bulk_load = 1', 'SET @rocksdb_dummy_bulk_load = 0') */;
/*!50717 PREPARE s FROM @rocksdb_enable_bulk_load */;
/*!50717 EXECUTE s */;
/*!50717 DEALLOCATE PREPARE s */;

--
-- Current Database: `laser`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `laser` /*!40100 DEFAULT CHARACTER SET utf8 */ /*!80016 DEFAULT ENCRYPTION='N' */;

USE `laser`;

--
-- Table structure for table `ansible_configs`
--

DROP TABLE IF EXISTS `ansible_configs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `ansible_configs` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `vars` text NOT NULL,
  `roles` text NOT NULL,
  `desc` varchar(256) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ansible_configs`
--

LOCK TABLES `ansible_configs` WRITE;
/*!40000 ALTER TABLE `ansible_configs` DISABLE KEYS */;
INSERT INTO `ansible_configs` VALUES (1,'test','[]','laser','','2020-10-28 08:01:48');
/*!40000 ALTER TABLE `ansible_configs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `cluster_running_metrics`
--

DROP TABLE IF EXISTS `cluster_running_metrics`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `cluster_running_metrics` (
  `id` int NOT NULL AUTO_INCREMENT,
  `qps` int DEFAULT NULL,
  `kps` int DEFAULT NULL,
  `kps_write` int DEFAULT NULL,
  `kps_read` int DEFAULT NULL,
  `bps` bigint DEFAULT NULL,
  `bps_write` bigint DEFAULT NULL,
  `bps_read` bigint DEFAULT NULL,
  `data_size` bigint DEFAULT NULL,
  `p99` float DEFAULT NULL,
  `p999` float DEFAULT NULL,
  `jitter_duration` int DEFAULT NULL,
  `capacity_idle_rate` float DEFAULT NULL,
  `capacity_idle_rate_absolute` float DEFAULT NULL,
  `collect_time` timestamp NULL DEFAULT NULL,
  `time_type` int DEFAULT NULL COMMENT '1-平峰，2-高峰',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `cluster_running_metrics`
--

LOCK TABLES `cluster_running_metrics` WRITE;
/*!40000 ALTER TABLE `cluster_running_metrics` DISABLE KEYS */;
/*!40000 ALTER TABLE `cluster_running_metrics` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `clusters`
--

DROP TABLE IF EXISTS `clusters`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `clusters` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `alias` varchar(255) NOT NULL,
  `shard_total` int NOT NULL,
  `desc` varchar(256) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uix_clusters_name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `clusters`
--

LOCK TABLES `clusters` WRITE;
/*!40000 ALTER TABLE `clusters` DISABLE KEYS */;
INSERT INTO `clusters` VALUES (1,'test','test',10,'测试','2020-10-28 06:51:07');
/*!40000 ALTER TABLE `clusters` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `dcs`
--

DROP TABLE IF EXISTS `dcs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `dcs` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `shard_number` int DEFAULT NULL,
  `desc` varchar(255) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  `cluster_id` int NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uix_dcs_name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=16 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `dcs`
--

LOCK TABLES `dcs` WRITE;
/*!40000 ALTER TABLE `dcs` DISABLE KEYS */;
INSERT INTO `dcs` VALUES (1,'default',10,'默认数据中心',NULL,1);
/*!40000 ALTER TABLE `dcs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `group_running_metrics`
--

DROP TABLE IF EXISTS `group_running_metrics`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `group_running_metrics` (
  `id` int NOT NULL AUTO_INCREMENT,
  `group_name` varchar(255) DEFAULT NULL,
  `qps` int DEFAULT NULL,
  `kps` int DEFAULT NULL,
  `kps_write` int DEFAULT NULL,
  `kps_read` int DEFAULT NULL,
  `bps` bigint DEFAULT NULL,
  `bps_write` bigint DEFAULT NULL,
  `bps_read` bigint DEFAULT NULL,
  `data_size` bigint DEFAULT NULL,
  `p99` float DEFAULT NULL,
  `p999` float DEFAULT NULL,
  `jitter_duration` int DEFAULT NULL,
  `capacity_idle_rate` float DEFAULT NULL,
  `capacity_idle_rate_absolute` float DEFAULT NULL,
  `collect_time` timestamp NULL DEFAULT NULL,
  `time_type` int DEFAULT NULL COMMENT '1-平峰，2-高峰',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `group_running_metrics`
--

LOCK TABLES `group_running_metrics` WRITE;
/*!40000 ALTER TABLE `group_running_metrics` DISABLE KEYS */;
/*!40000 ALTER TABLE `group_running_metrics` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `groups`
--

DROP TABLE IF EXISTS `groups`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `groups` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `alias` varchar(255) NOT NULL,
  `node_config_id` int NOT NULL,
  `desc` varchar(256) DEFAULT NULL,
  `cluster_id` int NOT NULL,
  `dc_id` int NOT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uix_groups_name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `groups`
--

LOCK TABLES `groups` WRITE;
/*!40000 ALTER TABLE `groups` DISABLE KEYS */;
INSERT INTO `groups` VALUES (1,'default','default',1,'',1,1,'2020-10-28 07:52:48');
/*!40000 ALTER TABLE `groups` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `laser_databases`
--

DROP TABLE IF EXISTS `laser_databases`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `laser_databases` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `desc` varchar(256) NOT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `laser_databases`
--

LOCK TABLES `laser_databases` WRITE;
/*!40000 ALTER TABLE `laser_databases` DISABLE KEYS */;
INSERT INTO `laser_databases` VALUES (1,'test_database','','2020-10-28 07:52:57');
/*!40000 ALTER TABLE `laser_databases` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `machine_categories`
--

DROP TABLE IF EXISTS `machine_categories`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `machine_categories` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `desc` varchar(256) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uix_machine_categories_name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `machine_categories`
--

LOCK TABLES `machine_categories` WRITE;
/*!40000 ALTER TABLE `machine_categories` DISABLE KEYS */;
INSERT INTO `machine_categories` VALUES (1,'docker','','2020-10-28 07:50:25');
/*!40000 ALTER TABLE `machine_categories` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `machines`
--

DROP TABLE IF EXISTS `machines`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `machines` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `category_id` int NOT NULL,
  `ip` varchar(255) NOT NULL,
  `cpu_core_number` varchar(255) NOT NULL,
  `memory_size_gb` varchar(255) NOT NULL,
  `desc` varchar(256) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uix_machines_ip` (`ip`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `machines`
--

LOCK TABLES `machines` WRITE;
/*!40000 ALTER TABLE `machines` DISABLE KEYS */;
INSERT INTO `machines` VALUES (1,1,'172.16.1.6','4','12','','2020-10-28 07:50:48'),(2,1,'172.16.1.7','4','12','','2020-10-28 07:50:48');
/*!40000 ALTER TABLE `machines` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `node_configs`
--

DROP TABLE IF EXISTS `node_configs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `node_configs` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `desc` varchar(255) DEFAULT NULL,
  `block_cache_size_gb` int NOT NULL,
  `write_buffer_size_gb` int NOT NULL,
  `num_shard_bits` int NOT NULL,
  `high_pri_pool_ratio` double NOT NULL,
  `strict_capacity_limit` int NOT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `node_configs`
--

LOCK TABLES `node_configs` WRITE;
/*!40000 ALTER TABLE `node_configs` DISABLE KEYS */;
INSERT INTO `node_configs` VALUES (1,'default','',1,1,6,0.5,1,'2020-10-28 07:52:34');
/*!40000 ALTER TABLE `node_configs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `node_physical_metrics`
--

DROP TABLE IF EXISTS `node_physical_metrics`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `node_physical_metrics` (
  `id` int NOT NULL AUTO_INCREMENT,
  `service_address` varchar(255) DEFAULT NULL,
  `cpu_usage_rate` float DEFAULT NULL,
  `cpu_usage_rate_user` float DEFAULT NULL,
  `cpu_usage_rate_system` float DEFAULT NULL,
  `memory_size_total` bigint DEFAULT NULL,
  `memory_size_usage` bigint DEFAULT NULL,
  `memory_usage_rate` float DEFAULT NULL,
  `disk_size_total` bigint DEFAULT NULL,
  `disk_size_usage` bigint DEFAULT NULL,
  `disk_usage_rate` float DEFAULT NULL,
  `io_usage_rate` float DEFAULT NULL,
  `network_in` bigint DEFAULT NULL,
  `network_out` bigint DEFAULT NULL,
  `collect_time` timestamp NULL DEFAULT NULL,
  `time_type` int DEFAULT NULL COMMENT '1-平峰，2-高峰',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `node_physical_metrics`
--

LOCK TABLES `node_physical_metrics` WRITE;
/*!40000 ALTER TABLE `node_physical_metrics` DISABLE KEYS */;
/*!40000 ALTER TABLE `node_physical_metrics` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `node_rate_limit_entries`
--

DROP TABLE IF EXISTS `node_rate_limit_entries`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `node_rate_limit_entries` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `begin_hour` int NOT NULL,
  `end_hour` int NOT NULL,
  `rate_bytes_per_sec` bigint NOT NULL,
  `node_config_id` int NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `node_rate_limit_entries`
--

LOCK TABLES `node_rate_limit_entries` WRITE;
/*!40000 ALTER TABLE `node_rate_limit_entries` DISABLE KEYS */;
/*!40000 ALTER TABLE `node_rate_limit_entries` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `node_running_metrics`
--

DROP TABLE IF EXISTS `node_running_metrics`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `node_running_metrics` (
  `id` int NOT NULL AUTO_INCREMENT,
  `service_address` varchar(255) DEFAULT NULL,
  `qps` int DEFAULT NULL,
  `kps` int DEFAULT NULL,
  `kps_write` int DEFAULT NULL,
  `kps_read` int DEFAULT NULL,
  `bps` bigint DEFAULT NULL,
  `bps_write` bigint DEFAULT NULL,
  `bps_read` bigint DEFAULT NULL,
  `data_size` bigint DEFAULT NULL,
  `p99` float DEFAULT NULL,
  `p999` float DEFAULT NULL,
  `jitter_duration` int DEFAULT NULL,
  `capacity_idle_rate` float DEFAULT NULL,
  `capacity_idle_rate_absolute` float DEFAULT NULL,
  `collect_time` timestamp NULL DEFAULT NULL,
  `time_type` int DEFAULT NULL COMMENT '1-平峰，2-高峰',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `node_running_metrics`
--

LOCK TABLES `node_running_metrics` WRITE;
/*!40000 ALTER TABLE `node_running_metrics` DISABLE KEYS */;
/*!40000 ALTER TABLE `node_running_metrics` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `nodes`
--

DROP TABLE IF EXISTS `nodes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `nodes` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `node_id` int NOT NULL,
  `active` int NOT NULL,
  `status` int NOT NULL,
  `host` varchar(256) NOT NULL,
  `port` int NOT NULL,
  `weight` int NOT NULL,
  `master` tinyint(1) NOT NULL DEFAULT '0',
  `is_edge_node` tinyint(1) NOT NULL DEFAULT '0',
  `desc` varchar(256) DEFAULT NULL,
  `group_id` int NOT NULL,
  `config_id` int NOT NULL,
  `leader_shard_list` text,
  `follower_shard_list` text,
  `ansible_config_id` int NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `nodes`
--

LOCK TABLES `nodes` WRITE;
/*!40000 ALTER TABLE `nodes` DISABLE KEYS */;
INSERT INTO `nodes` VALUES (1,1,1,1,'172.16.1.6',10022,0,1,0,'',1,1,'0,1,2,3,4,5,6,7,8,9','',1),(2,2,1,1,'172.16.1.7',10023,0,1,0,'',1,1,'','0,1,2,3,4,5,6,7,8,9',1);
/*!40000 ALTER TABLE `nodes` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `proxy_table_configs`
--

DROP TABLE IF EXISTS `proxy_table_configs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `proxy_table_configs` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `database_name` varchar(255) NOT NULL,
  `table_name` varchar(255) NOT NULL,
  `read_timeout` int NOT NULL,
  `write_timeout` int NOT NULL,
  `allowed_flow` int NOT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `proxy_table_configs`
--

LOCK TABLES `proxy_table_configs` WRITE;
/*!40000 ALTER TABLE `proxy_table_configs` DISABLE KEYS */;
INSERT INTO `proxy_table_configs` VALUES (1,'global','global',10,10,100,'2020-10-29 03:06:42');
/*!40000 ALTER TABLE `proxy_table_configs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `resource_pool_running_metrics`
--

DROP TABLE IF EXISTS `resource_pool_running_metrics`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `resource_pool_running_metrics` (
  `id` int NOT NULL AUTO_INCREMENT,
  `pool_name` varchar(255) DEFAULT NULL,
  `qps` int DEFAULT NULL,
  `kps` int DEFAULT NULL,
  `kps_write` int DEFAULT NULL,
  `kps_read` int DEFAULT NULL,
  `bps` bigint DEFAULT NULL,
  `bps_write` bigint DEFAULT NULL,
  `bps_read` bigint DEFAULT NULL,
  `data_size` bigint DEFAULT NULL,
  `p99` float DEFAULT NULL,
  `p999` float DEFAULT NULL,
  `jitter_duration` int DEFAULT NULL,
  `capacity_idle_rate` float DEFAULT NULL,
  `capacity_idle_rate_absolute` float DEFAULT NULL,
  `collect_time` timestamp NULL DEFAULT NULL,
  `time_type` int DEFAULT NULL COMMENT '1-平峰，2-高峰',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `resource_pool_running_metrics`
--

LOCK TABLES `resource_pool_running_metrics` WRITE;
/*!40000 ALTER TABLE `resource_pool_running_metrics` DISABLE KEYS */;
/*!40000 ALTER TABLE `resource_pool_running_metrics` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `shard_stores`
--

DROP TABLE IF EXISTS `shard_stores`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `shard_stores` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `service_name` varchar(256) NOT NULL,
  `version` varchar(256) NOT NULL,
  `data` text NOT NULL,
  `status` int unsigned NOT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `shard_stores`
--

LOCK TABLES `shard_stores` WRITE;
/*!40000 ALTER TABLE `shard_stores` DISABLE KEYS */;
/*!40000 ALTER TABLE `shard_stores` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `system_indices`
--

DROP TABLE IF EXISTS `system_indices`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `system_indices` (
  `id` int NOT NULL AUTO_INCREMENT,
  `io_rate_per_mb` float DEFAULT NULL,
  `net_amplification_ratio` float DEFAULT NULL,
  `disk_amplification_ratio` float DEFAULT NULL,
  `collect_time` timestamp NULL DEFAULT NULL,
  `time_type` int DEFAULT NULL COMMENT '0-平峰，1-高峰',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `system_indices`
--

LOCK TABLES `system_indices` WRITE;
/*!40000 ALTER TABLE `system_indices` DISABLE KEYS */;
/*!40000 ALTER TABLE `system_indices` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `table_config_items`
--

DROP TABLE IF EXISTS `table_config_items`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `table_config_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `value` varchar(256) NOT NULL,
  `type` int NOT NULL,
  `config_id` int NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `name_config_id` (`name`,`config_id`)
) ENGINE=InnoDB AUTO_INCREMENT=18 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `table_config_items`
--

LOCK TABLES `table_config_items` WRITE;
/*!40000 ALTER TABLE `table_config_items` DISABLE KEYS */;
INSERT INTO `table_config_items` VALUES (1,'WAL_size_limit_MB','512',1,1),(2,'WAL_ttl_seconds','3600',1,1),(3,'compaction_readahead_size','2097152',1,1),(4,'create_if_missing','true',1,1),(5,'max_background_compactions','20',1,1),(6,'max_background_flushes','20',1,1),(7,'use_direct_reads','true',1,1),(8,'block_size','65536',2,1),(9,'cache_index_and_filter_blocks','true',2,1),(10,'cache_index_and_filter_blocks_with_high_priority','true',2,1),(11,'data_block_index_type','kDataBlockBinaryAndHash',2,1),(12,'filter_policy','bloomfilter:10:false',2,1),(13,'index_type','kTwoLevelIndexSearch',2,1),(14,'metadata_block_size','4096',2,1),(15,'partition_filters','true',2,1),(16,'pin_l0_filter_and_index_blocks_in_cache','true',2,1),(17,'num_levels','7',3,1);
/*!40000 ALTER TABLE `table_config_items` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `table_configs`
--

DROP TABLE IF EXISTS `table_configs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `table_configs` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `version` int NOT NULL,
  `is_default` int NOT NULL,
  `desc` varchar(256) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `table_configs`
--

LOCK TABLES `table_configs` WRITE;
/*!40000 ALTER TABLE `table_configs` DISABLE KEYS */;
INSERT INTO `table_configs` VALUES (1,'default',1,1,'默认表配置');
/*!40000 ALTER TABLE `table_configs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `table_edge_nodes`
--

DROP TABLE IF EXISTS `table_edge_nodes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `table_edge_nodes` (
  `table_id` int unsigned NOT NULL,
  `node_id` int unsigned NOT NULL,
  PRIMARY KEY (`table_id`,`node_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `table_edge_nodes`
--

LOCK TABLES `table_edge_nodes` WRITE;
/*!40000 ALTER TABLE `table_edge_nodes` DISABLE KEYS */;
/*!40000 ALTER TABLE `table_edge_nodes` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `table_metrics`
--

DROP TABLE IF EXISTS `table_metrics`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `table_metrics` (
  `id` int NOT NULL AUTO_INCREMENT,
  `database_name` varchar(255) DEFAULT NULL,
  `table_name` varchar(255) DEFAULT NULL,
  `data_size` bigint DEFAULT NULL,
  `kps` int DEFAULT NULL,
  `kps_write` int DEFAULT NULL,
  `kps_read` int DEFAULT NULL,
  `bps` bigint DEFAULT NULL,
  `bps_write` bigint DEFAULT NULL,
  `bps_read` bigint DEFAULT NULL,
  `partition_number` int DEFAULT NULL,
  `collect_time` timestamp NULL DEFAULT NULL,
  `time_type` int DEFAULT NULL COMMENT '0-平峰，1-高峰',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `table_metrics`
--

LOCK TABLES `table_metrics` WRITE;
/*!40000 ALTER TABLE `table_metrics` DISABLE KEYS */;
/*!40000 ALTER TABLE `table_metrics` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tables`
--

DROP TABLE IF EXISTS `tables`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `tables` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `status` int NOT NULL,
  `deny_all` int NOT NULL,
  `partition_number` int NOT NULL,
  `ttl` bigint unsigned NOT NULL,
  `desc` varchar(256) DEFAULT NULL,
  `dc_id` int NOT NULL,
  `dist_dc_id` int NOT NULL,
  `database_id` int NOT NULL,
  `config_id` int NOT NULL,
  `edge_flow_ratio` int NOT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tables`
--

LOCK TABLES `tables` WRITE;
/*!40000 ALTER TABLE `tables` DISABLE KEYS */;
INSERT INTO `tables` VALUES (1,'test_table',1,0,10,0,'',1,1,1,1,0,'2020-10-28 07:53:48'),(2,'test_dc',1,0,10,0,'',1,1,1,1,0,'2020-10-28 07:54:15');
/*!40000 ALTER TABLE `tables` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tickets`
--

DROP TABLE IF EXISTS `tickets`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `tickets` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `creator` varchar(255) NOT NULL,
  `sdk_type` varchar(255) NOT NULL,
  `read_contact_persion` varchar(255) NOT NULL,
  `write_contact_persion` varchar(255) NOT NULL,
  `import_data_contact_persion` varchar(255) NOT NULL,
  `business_line` varchar(255) NOT NULL,
  `business_description` varchar(255) NOT NULL,
  `business_kr` varchar(255) NOT NULL,
  `database_name` varchar(255) NOT NULL,
  `table_name` varchar(255) NOT NULL,
  `read_qps` int NOT NULL,
  `write_qps` int NOT NULL,
  `request_delay_limit` int NOT NULL,
  `data_expiration_time` bigint NOT NULL,
  `data_size` int NOT NULL,
  `value_size` int NOT NULL,
  `crash_influence` varchar(255) NOT NULL,
  `command` varchar(255) NOT NULL,
  `key_num` int NOT NULL,
  `docking_personnel` varchar(255) NOT NULL,
  `status` int NOT NULL,
  `partition_num` int NOT NULL,
  `created_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tickets`
--

LOCK TABLES `tickets` WRITE;
/*!40000 ALTER TABLE `tickets` DISABLE KEYS */;
/*!40000 ALTER TABLE `tickets` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `traffic_restriction_limits`
--

DROP TABLE IF EXISTS `traffic_restriction_limits`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `traffic_restriction_limits` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `operation_type` int NOT NULL,
  `limit_type` int NOT NULL,
  `limit_value` int NOT NULL,
  `table_id` int NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `name_table_id` (`name`,`table_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `traffic_restriction_limits`
--

LOCK TABLES `traffic_restriction_limits` WRITE;
/*!40000 ALTER TABLE `traffic_restriction_limits` DISABLE KEYS */;
/*!40000 ALTER TABLE `traffic_restriction_limits` ENABLE KEYS */;
UNLOCK TABLES;
/*!50112 SET @disable_bulk_load = IF (@is_rocksdb_supported, 'SET SESSION rocksdb_bulk_load = @old_rocksdb_bulk_load', 'SET @dummy_rocksdb_bulk_load = 0') */;
/*!50112 PREPARE s FROM @disable_bulk_load */;
/*!50112 EXECUTE s */;
/*!50112 DEALLOCATE PREPARE s */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2020-12-18 14:55:59
