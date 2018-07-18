USE eos;

ALTER TABLE accounts_keys DROP FOREIGN KEY accounts_keys_ibfk_1;
ALTER TABLE actions DROP FOREIGN KEY actions_ibfk_1;
ALTER TABLE actions DROP FOREIGN KEY actions_ibfk_2;
ALTER TABLE actions_accounts DROP FOREIGN KEY actions_accounts_ibfk_1;
ALTER TABLE actions_accounts DROP FOREIGN KEY actions_accounts_ibfk_2;
ALTER TABLE blocks DROP FOREIGN KEY blocks_ibfk_1;
ALTER TABLE stakes DROP FOREIGN KEY stakes_ibfk_1;
ALTER TABLE tokens DROP FOREIGN KEY tokens_ibfk_1;
ALTER TABLE transactions DROP FOREIGN KEY transactions_ibfk_1;
ALTER TABLE votes DROP FOREIGN KEY votes_ibfk_1;

ALTER TABLE `tokens` CHANGE `amount` `amount` double(64,4) NOT NULL DEFAULT 0.0000;
ALTER TABLE `accounts_keys` CHANGE COLUMN `public_key` `public_key` varchar(64) NOT NULL DEFAULT '';

ALTER TABLE `actions` ADD COLUMN `eosto` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions` ADD COLUMN `eosfrom` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions` ADD COLUMN `receiver` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions` ADD COLUMN `payer` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions` ADD COLUMN `newaccount` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions` CHANGE COLUMN `name` `name` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';

ALTER TABLE `actions` ADD INDEX `idx_actions_name` (`name`);
ALTER TABLE `actions` ADD INDEX `idx_actions_eosto` (`eosto`);
ALTER TABLE `actions` ADD INDEX `idx_actions_eosfrom` (`eosfrom`);
ALTER TABLE `actions` ADD INDEX `idx_actions_receiver` (`receiver`);
ALTER TABLE `actions` ADD INDEX `idx_actions_payer` (`payer`);
ALTER TABLE `actions` ADD INDEX `idx_actions_newaccount` (`newaccount`);

ALTER TABLE `accounts` DROP PRIMARY KEY;
ALTER TABLE `accounts` ADD COLUMN `id` bigint(20) NOT NULL primary key auto_increment first;
ALTER TABLE `accounts` ADD INDEX `idx_accounts_name`(`name`);
ALTER TABLE `accounts` CHANGE COLUMN `name` `name` varchar(12) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `accounts` CHANGE COLUMN `created_at` `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP;
ALTER TABLE `accounts` CHANGE COLUMN `updated_at` `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP;

ALTER TABLE `accounts_keys` ADD COLUMN `id` bigint(20) NOT NULL primary key auto_increment first;
ALTER TABLE `accounts_keys` CHANGE COLUMN `account` `account` varchar(16) NOT NULL DEFAULT '';
ALTER TABLE `accounts_keys` CHANGE COLUMN `permission` `permission` varchar(16) NOT NULL DEFAULT '';

ALTER TABLE `actions` CHANGE COLUMN `id` `id` bigint(20) NOT NULL auto_increment first;
ALTER TABLE `actions` CHANGE COLUMN `account` `account` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions` CHANGE COLUMN `transaction_id` `transaction_id` varchar(64) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions` CHANGE COLUMN `seq` `seq` smallint(6) NOT NULL DEFAULT 0;
ALTER TABLE `actions` CHANGE COLUMN `parent` `parent` bigint(20) NOT NULL DEFAULT 0;
ALTER TABLE `actions` CHANGE COLUMN `name` `name` varchar(12) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions` CHANGE COLUMN `created_at` `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP;

ALTER TABLE `actions_accounts` ADD COLUMN `id` bigint(20) NOT NULL primary key auto_increment first;
ALTER TABLE `actions_accounts` CHANGE COLUMN `actor` `actor` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions_accounts` CHANGE COLUMN `permission` `permission` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `actions_accounts` CHANGE COLUMN `action_id` `action_id` bigint(20) NOT NULL DEFAULT 0;

ALTER TABLE `blocks` CHANGE COLUMN `block_number` `block_number` bigint(20) NOT NULL AUTO_INCREMENT;
ALTER TABLE `blocks` CHANGE COLUMN `id` `id` varchar(64) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `blocks` CHANGE COLUMN `prev_block_id` `prev_block_id` varchar(64) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `blocks` CHANGE COLUMN `irreversible` `irreversible` tinyint(1) NOT NULL DEFAULT '0';
ALTER TABLE `blocks` CHANGE COLUMN `timestamp` `timestamp` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP;
ALTER TABLE `blocks` CHANGE COLUMN `transaction_merkle_root` `transaction_merkle_root` varchar(64) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `blocks` CHANGE COLUMN `action_merkle_root` `action_merkle_root` varchar(64) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `blocks` CHANGE COLUMN `producer` `producer` varchar(12) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `blocks` CHANGE COLUMN `version` `version` bigint(20) NOT NULL DEFAULT '0';
ALTER TABLE `blocks` CHANGE COLUMN `num_transactions` `num_transactions` bigint(20) NOT NULL DEFAULT '0';
ALTER TABLE `blocks` CHANGE COLUMN `confirmed` `confirmed` bigint(20) NOT NULL DEFAULT '0';

ALTER TABLE `stakes` DROP PRIMARY KEY;
ALTER TABLE `stakes` ADD COLUMN `id` bigint(20) NOT NULL primary key auto_increment first;
ALTER TABLE `stakes` ADD UNIQUE INDEX `idx_stakes_account` (`account`);
ALTER TABLE `stakes` CHANGE COLUMN `account` `account` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `stakes` CHANGE COLUMN `cpu` `cpu` double(14,4) NOT NULL DEFAULT 0.0000;
ALTER TABLE `stakes` CHANGE COLUMN `net` `net` double(14,4) NOT NULL DEFAULT 0.0000;

ALTER TABLE `tokens` ADD COLUMN `id` bigint(20) NOT NULL primary key auto_increment first;
ALTER TABLE `tokens` CHANGE COLUMN `account` `account` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `tokens` CHANGE COLUMN `symbol` `symbol` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
ALTER TABLE `tokens` CHANGE COLUMN `amount` `amount` double(64,4) NOT NULL DEFAULT 0.0000;

ALTER TABLE `transactions` DROP PRIMARY KEY;
ALTER TABLE `transactions` ADD COLUMN `tx_id` bigint(20) NOT NULL primary key auto_increment first;
ALTER TABLE `transactions` ADD UNIQUE INDEX `idx_transactions_id` (`id`);
ALTER TABLE `transactions` CHANGE COLUMN `block_id` `block_id` bigint(20) NOT NULL DEFAULT '0';
ALTER TABLE `transactions` CHANGE COLUMN `ref_block_num` `ref_block_num` bigint(20) NOT NULL DEFAULT '0';
ALTER TABLE `transactions` CHANGE COLUMN `ref_block_prefix` `ref_block_prefix` bigint(20) NOT NULL DEFAULT '0';
ALTER TABLE `transactions` CHANGE COLUMN `expiration` `expiration` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP;
ALTER TABLE `transactions` CHANGE COLUMN `pending` `pending` tinyint(1) NOT NULL DEFAULT '0';
ALTER TABLE `transactions` CHANGE COLUMN `created_at` `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP;
ALTER TABLE `transactions` CHANGE COLUMN `num_actions` `num_actions` bigint(20) DEFAULT '0';
ALTER TABLE `transactions` CHANGE COLUMN `updated_at` `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP;

ALTER TABLE `votes` DROP PRIMARY KEY;
ALTER TABLE `votes` ADD COLUMN `id` bigint(20) NOT NULL primary key auto_increment first;
ALTER TABLE `votes` CHANGE COLUMN `account` `account` varchar(16) COLLATE utf8mb4_general_ci NOT NULL DEFAULT '';
