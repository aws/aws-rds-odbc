# Authentication
The AWS RDS ODBC Library streamlines the process of generating, retrieving, and caching authentication credentials for connecting to AWS RDS databases. Users are required to manage memory buffers for credential generation, while the library is designed to be compatible with various compilers and supports both C and C++ programming languages.

# Supported Authentication Methods
Currently, the library supports the following methods of generating credentials:
- [AWS IAM Authentication](#aws-iam-authentication)
- [AWS Secrets Manager](#aws-secrets-manager)
- [Active Directory Federated Services (ADFS) Authentication](#active-directory-federated-services-authentication-adfs)
- [Okta Authentication](#okta-authentication)

## AWS IAM Authentication
AWS Identity and Access Management (IAM) grants users access control across all Amazon Web Services. IAM supports granular permissions, giving you the ability to grant different permissions to different users. For more information on IAM and its use cases, please refer to the [IAM documentation](https://docs.aws.amazon.com/IAM/latest/UserGuide/introduction.html).

The AWS RDS ODBC Library allows users to authenticate with IAM and generate a session token. This token can then be used to authenticate to an AWS RDS database.

### Prerequisites to using AWS IAM database authentication
1. Enable AWS IAM database authentication:
    1. If needed, review the documentation about [creating a new database](https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/USER_CreateDBInstance.html).
    1. If needed, review the documentation about [modifying an existing database](https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Overview.DBInstance.Modifying.html).
1. Set up an AWS IAM policy for AWS IAM database authentication:
    1. Refer to the [AWS IAM Policy documentation](https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/UsingWithRDS.IAMDBAuth.IAMPolicy.html) for setting up the policy.
1. Create an IAM Database Account:
    1. Follow the steps in [Creating a Database Account]((https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/UsingWithRDS.IAMDBAuth.DBAccounts.html))

### Sample IAM Code
[AWS IAM Example](./auth_iam_example.cpp)

## AWS Secrets Manager
AWS Secrets Manager helps manage, retrieve, and rotate database credentials. Secret Manager can also further grant granulated access through [AWS Identity and Access Management (IAM)](https://docs.aws.amazon.com/secretsmanager/latest/userguide/intro.html). For more information on Secret Manager and its use cases, please refer to the [Secret Manager documentation](https://docs.aws.amazon.com/secretsmanager/latest/userguide/intro.html).

The AWS RDS ODBC Library provides users a way to retrieve the database username and password from AWS Secrets Manager.

### Sample Secrets Manager Code
[AWS Secrets Manager Example](./auth_secrets_manager_example.cpp)

## Active Directory Federated Services Authentication (ADFS)
Federated Identity allows users to use the same set of credentials to access multiple services or resources across different organizations. This is achieved by having Identity Providers (IdP) that manage and authenticate user credentials and Service Providers (SP) that are services or resources that can be internal, external, and/or belonging to various organizations. Multiple SPs can establish trust relationships with a single IdP.

When a user wants access to a resource, they authenticate with the IdP. A security token is generated and passed to the SP, which then grants access to the resource. In the case of ADFS, the user signs into the ADFS sign-in page, generating a SAML Assertion that acts as a security token. The user then passes the SAML Assertion to the SP when requesting access to resources. The SP verifies the SAML Assertion and grants access to the user.

The AWS RDS ODBC Library allows users to authenticate with ADFS and generate a session token, which can then be used to authenticate to an AWS RDS database.

## Required ADFS FederatedAuthConfig

| Field | Description | Example
|-|-|-|
| idp_endpoint | The hosting URL for the service that you are using to authenticate into AWS Aurora. | https://www.example.com |
| idp_port | The port that the host for the authentication service listens at | 1234 |
| relaying_party_id | The Uniform Resource Name (URN) to connect to | urn:amazon:webservices |
| iam_role_arn | The ARN of the IAM Role that is to be assumed to access AWS Aurora. | arn:aws:iam::123412341234:role/adfs_example_role |
| iam_idp_arn | The ARN of the Identity Provider. |  arn:aws:iam::123412341234:saml-provider/adfs_example_provider |
| idp_username | The username for the idp_endpoint server | user@email.com |
| idp_password | The password for the idp_endpoint server | email_password |

### Sample ADFS Code
[ADFS Example](./auth_adfs_example.cpp)

## Okta Authentication
The driver supports authentication via an [Okta](https://www.okta.com/) federated identity and then database access via IAM.

## Required Okta FederatedAuthConfig

| Field | Description | Example
|-|-|-|
| idp_endpoint | The hosting URL for the service that you are using to authenticate into AWS Aurora. | https://www.example.com |
| idp_port | The port that the host for the authentication service listens at | 1234 |
| app_id | The Amazon Web Services (AWS) app [configured on Okta](https://help.okta.com/en-us/content/topics/deploymentguides/aws/aws-configure-aws-app.htm) | my-app-id |
| iam_role_arn | The ARN of the IAM Role that is to be assumed to access AWS Aurora. | arn:aws:iam::123412341234:role/okta_example_role |
| iam_idp_arn | The ARN of the Identity Provider |  arn:aws:iam::123412341234:saml-provider/okta_example_provider |
| idp_username | The username for the idp_endpoint server | user@email.com |
| idp_password | The password for the idp_endpoint server | email_password |

### Sample Okta Code
[Okta Example](./auth_okta_example.cpp)
