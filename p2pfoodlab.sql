SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

CREATE DATABASE IF NOT EXISTS `p2pfoodlab` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci;
USE `p2pfoodlab`;

CREATE TABLE IF NOT EXISTS `accounts` (
  `id` int(64) NOT NULL AUTO_INCREMENT,
  `username` varchar(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `email` varchar(255) CHARACTER SET ascii NOT NULL,
  `pwhash` varchar(255) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `key` varchar(255) CHARACTER SET ascii COLLATE ascii_bin DEFAULT NULL,
  `validated` int(11) NOT NULL,
  `code` varchar(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `locale` varchar(3) CHARACTER SET ascii COLLATE ascii_bin NOT NULL DEFAULT 'en',
  `osd_id` int(11) DEFAULT NULL,
  `osd_username` varchar(64) CHARACTER SET ascii COLLATE ascii_bin DEFAULT NULL,
  `osd_key` varchar(255) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  UNIQUE KEY `id` (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 AUTO_INCREMENT=11 ;

CREATE TABLE IF NOT EXISTS `account_lang` (
  `id` int(11) NOT NULL,
  `lang` varchar(5) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `order` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`,`lang`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `account_posts` (
  `account_id` int(11) NOT NULL,
  `post_id` int(11) NOT NULL,
  `lang` varchar(3) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  PRIMARY KEY (`account_id`,`post_id`,`lang`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `account_sensorbox` (
  `account_id` int(11) NOT NULL,
  `group_id` int(11) NOT NULL,
  UNIQUE KEY `account_id` (`account_id`,`group_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `channels` (
  `id` int(11) NOT NULL,
  `lang` varchar(3) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `title` varchar(100) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `order` int(11) NOT NULL,
  UNIQUE KEY `id_2` (`id`,`lang`),
  KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `channel_posts` (
  `channel_id` int(11) NOT NULL,
  `post_id` int(11) NOT NULL,
  `lang` varchar(3) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `channel_tags` (
  `channel_id` int(11) NOT NULL,
  `tag_id` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `docs` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `path` varchar(32) NOT NULL,
  `type` varchar(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=27 ;

CREATE TABLE IF NOT EXISTS `locale` (
  `code` varchar(3) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `group` varchar(255) COLLATE utf8_bin NOT NULL,
  `english` varchar(255) COLLATE utf8_bin NOT NULL,
  `native` varchar(255) COLLATE utf8_bin NOT NULL,
  `most_used` int(11) NOT NULL DEFAULT '0',
  `interface` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

CREATE TABLE IF NOT EXISTS `posts` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `from` int(11) NOT NULL,
  `thread` int(11) DEFAULT NULL,
  `replyto` int(11) DEFAULT NULL,
  `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `text` text CHARACTER SET utf8 COLLATE utf8_bin,
  `text_html` text CHARACTER SET utf8 COLLATE utf8_bin,
  `lang` varchar(2) CHARACTER SET ascii COLLATE ascii_bin DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=59 ;

CREATE TABLE IF NOT EXISTS `post_docs` (
  `post_id` int(11) NOT NULL,
  `doc_id` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `sensorbox_datastreams` (
  `group_id` int(11) NOT NULL,
  `datastream_id` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `tags` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `text` varchar(128) CHARACTER SET utf8 NOT NULL,
  `lang` varchar(3) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=43 ;

CREATE TABLE IF NOT EXISTS `tag_posts` (
  `tag_id` int(11) NOT NULL,
  `post_id` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
