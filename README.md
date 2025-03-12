# Amazon Web Services (AWS) RDS Library for ODBC Drivers

[![License](https://img.shields.io/badge/license-Apachev2-blue)](LICENSE)

The Amazon Web Services (AWS) RDS Library for ODBC drivers allow the Amazon Web Services (AWS) ODBC drivers to take
advantage of AWS RDS functionality inside an Amazon Aurora database (DB) cluster.

This library currently supports the [AWS ODBC Driver for PostgreSQL](https://github.com/aws/aws-pgsql-odbc).

## Table of Contents

- [Amazon Web Services (AWS) RDS Library for ODBC Drivers](#amazon-web-services-aws-rds-library-for-odbc-drivers)
  - [Table of Contents](#table-of-contents)
  - [About the Library](#about-the-library)
    - [Benefits of this library](#benefits-of-this-library)
  - [Documentation](#documentation)
  - [Getting Help and Opening Issues](#getting-help-and-opening-issues)
  - [License](#license)

## About the Library

### Benefits of this library

This library provides support for several AWS-specific features that are not available in the community [PostgreSQL](https://github.com/postgresql-interfaces/psqlodbc) and [MySQL](https://github.com/mysql/mysql-connector-odbc) ODBC drivers.
These features include:

1. IAM Authentication
2. Secrets Manager Authentication
3. Federated Authentication via ADFS and Okta
4. Aurora Limitless Database Support
5. Failover Support

This library is not intended to be used on its own and is intended to work by integrating it with the AWS ODBC Driver for PostgreSQL.
To learn more about how to use these features please visit the [AWS ODBC Driver for PostgreSQL GitHub page](https://github.com/aws/aws-pgsql-odbc).

## Documentation

Please refer to the [documentation](docs/Documentation.md) for details on how to build and test the library.

## Getting Help and Opening Issues

If you encounter an issue or bug while using the library with the AWS ODBC Driver for PostgreSQL, we would like to hear about it.
Please see the [Getting Help and Opening Issues](https://github.com/aws/aws-pgsql-odbc?tab=readme-ov-file#getting-help-and-opening-issues) section on the AWS ODBC Driver for PostgreSQL and submit the issue there.

## License

This software is released under the Apache 2.0 license.
