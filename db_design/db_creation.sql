-- MySQL dump 10.13  Distrib 5.7.31, for Linux (x86_64)
--
-- Host: localhost    Database: sandbox
-- ------------------------------------------------------
-- Server version	5.7.31-0ubuntu0.18.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `lightning_readings`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `lightning_readings` (
  `power` int(11) DEFAULT NULL,
  `timestamp` timestamp NULL DEFAULT NULL,
  `latitude` decimal(8,6) DEFAULT NULL,
  `longitude` decimal(9,6) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `lightning_readings`
--


--
-- Table structure for table `lightning_strikes`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `lightning_strikes` (
  `timestamp` timestamp NULL DEFAULT NULL,
  `latitude` decimal(8,6) DEFAULT NULL,
  `longitude` decimal(9,6) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `lightning_strikes`
--


--
-- Table structure for table `nodes`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `nodes` (
  `nodename` varchar(16) NOT NULL,
  `longitude` decimal(9,6) DEFAULT NULL,
  `latitude` decimal(8,6) DEFAULT NULL,
  `has_thermometer` tinyint(1) DEFAULT '0',
  `has_barometer` tinyint(1) DEFAULT '0',
  `has_humidity` tinyint(1) DEFAULT '0',
  `has_lightning` tinyint(1) DEFAULT '1',
  `has_anenometer` tinyint(1) DEFAULT '0',
  `has_weathervane` tinyint(1) DEFAULT '0',
  `has_raingauge` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`nodename`),
  UNIQUE KEY `nodes_nodename_uindex` (`nodename`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `nodes`
--


--
-- Table structure for table `weather_readings`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `weather_readings` (
  `temp` float DEFAULT NULL,
  `pressure` float DEFAULT NULL,
  `humidity` float DEFAULT NULL,
  `wind_direction` varchar(3) DEFAULT NULL,
  `wind_speed` float DEFAULT NULL,
  `rainfall` float DEFAULT NULL COMMENT 'Measured in mm',
  `nodename` varchar(16) NOT NULL,
  `latitude` decimal(8,6) DEFAULT NULL,
  `longitude` decimal(9,6) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `weather_readings`
--

/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2020-10-27  3:37:42
