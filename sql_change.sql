


ALTER TABLE `stakes` DROP COLUMN `cpu`;
ALTER TABLE `stakes` DROP COLUMN `net`;
ALTER TABLE `votes` change `account` `account` int;
ALTER TABLE `votes` DROP COLUMN `account`;
ALTER TABLE `votes` DROP COLUMN `votes`;

ALTER TABLE `stakes` ADD COLUMN `cpu_amount_for_self` double(64,4) DEFAULT '0.0000';
ALTER TABLE `stakes` ADD COLUMN `net_amount_for_self` double(64,4) DEFAULT '0.0000';
ALTER TABLE `stakes` ADD COLUMN `cpu_amount_for_other` double(64,4) DEFAULT '0.0000';
ALTER TABLE `stakes` ADD COLUMN `net_amount_for_other` double(64,4) DEFAULT '0.0000';
ALTER TABLE `tokens` ADD COLUMN `symbol_owner` varchar(30) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci DEFAULT NULL DEFAULT '';
ALTER TABLE `tokens` ADD COLUMN `symbol_owner_account` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `transactions` ADD COLUMN `irreversible` tinyint(1) NOT NULL DEFAULT '0';
ALTER TABLE `votes` ADD COLUMN `voter` varchar(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `votes` ADD COLUMN `proxy` varchar(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci DEFAULT NULL DEFAULT '';
ALTER TABLE `votes` ADD COLUMN `producers` json DEFAULT NULL;

ALETER TABLE `tokens` ADD UNIQUE KEY `idx_tokens_symbolowneraccount` (`symbol_owner_account`) USING BTREE;
ALETER TABLE `votes` ADD UNIQUE KEY `idx_votes_voter` (`voter`) USING BTREE; 

DROP TABLE IF EXISTS `assets`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
 SET character_set_client = utf8mb4 ;
CREATE TABLE `assets` (
  `symbol_owner` varchar(30) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `amount` double(64,30) DEFAULT NULL,
  `max_amount` double(64,30) DEFAULT NULL,
  `symbol_precision` int(2) DEFAULT NULL,
  `symbol` varchar(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL,
  `issuer` varchar(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL,
  `owner` varchar(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL,
  PRIMARY KEY (`symbol_owner`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;


DROP TABLE IF EXISTS `refunds`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
 SET character_set_client = utf8mb4 ;
CREATE TABLE `refunds` (
  `owner` varchar(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `time` timestamp NULL DEFAULT NULL,
  `net_amount` double(64,4) DEFAULT NULL,
  `cpu_amount` double(64,4) DEFAULT NULL,
  PRIMARY KEY (`owner`) USING BTREE,
  UNIQUE KEY `idx_owner` (`owner`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

DROP TABLE IF EXISTS `traces`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
 SET character_set_client = utf8mb4 ;
CREATE TABLE `traces` (
  `tx_id` bigint(20) NOT NULL AUTO_INCREMENT,
  `id` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL DEFAULT '',
  `data` json DEFAULT NULL,
  `irreversible` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`tx_id`),
  UNIQUE KEY `idx_transactions_id` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=80164 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;