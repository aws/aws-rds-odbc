# Amazon Web Services (AWS) RDS Library for ODBC Drivers

[![License](https://img.shields.io/badge/license-GPLv2-blue)](LICENSE)

The Amazon Web Services (AWS) RDS Library for ODBC drivers allow the Amazon Web Services (AWS) ODBC drivers to take
advantage of AWS RDS functionality like the failover functionality available inside an Amazon Aurora database (DB) cluster.

## Table of Contents
- [Amazon Web Services (AWS) RDS Library for ODBC Drivers](#amazon-web-services-aws-rds-library-for-odbc-drivers)
  - [Table of Contents](#table-of-contents)
  - [About the Library](#about-the-library)
    - [What is Failover?](#what-is-failover)
    - [Benefits of this library](#benefits-of-this-library)
  - [Getting Started](#getting-started)
  - [Building the library](#building-the-library)
  - [License](#license)

## About the Library

### What is Failover?
In an Amazon Aurora database (DB) cluster, failover is a mechanism by which Aurora automatically repairs the DB cluster status when a primary DB instance becomes unavailable. It achieves this goal by electing an Aurora Replica to become the new primary DB instance, so that the DB cluster can provide maximum availability to a primary read-write DB instance. The AWS ODBC Driver for MySQL is designed to coordinate with this behavior in order to provide minimal downtime in the event of a DB instance failure.

### Benefits of this library
Although Aurora is able to provide maximum availability through the use of failover, existing client drivers do not currently support this functionality. This is partially due to the time required for the DNS of the new primary DB instance to be fully resolved in order to properly direct the connection. The AWS Failover Library for ODBC drivers fully utilizes failover behavior by maintaining a cache of the Aurora cluster topology and each DB instance's role (Aurora Replica or primary DB instance). This topology is provided via a direct query to the Aurora database, essentially providing a shortcut to bypass the delays caused by DNS resolution. With this knowledge, this library can more closely monitor the Aurora DB cluster status so that a connection to the new primary DB instance can be established as fast as possible.

## Getting Started

### Building the library

#### Prequisites
1. Download the CMake Windows x64 Installer from https://cmake.org/download/ and install CMake using the installer. When going through the install, ensure that adding CMake to the PATH is selected.
1. Refer to [Install Microsoft Visual Studio](docs/InstallMicrosoftVisualStudio.md) to install Microsoft Visual Studio.

## License
This software is released under version 2 of the GNU General Public License (GPLv2).