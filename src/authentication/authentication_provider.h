// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2.0
// (GPLv2), as published by the Free Software Foundation, with the
// following additional permissions:
//
// This program is distributed with certain software that is licensed
// under separate terms, as designated in a particular file or component
// or in the license documentation. Without limiting your rights under
// the GPLv2, the authors of this program hereby grant you an additional
// permission to link the program and your derivative works with the
// separately licensed software that they have included with the program.
//
// Without limiting the foregoing grant of rights under the GPLv2 and
// additional permission as to separately licensed software, this
// program is also subject to the Universal FOSS Exception, version 1.0,
// a copy of which can be found along with its FAQ at
// http://oss.oracle.com/licenses/universal-foss-exception.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License, version 2.0, for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see 
// http://www.gnu.org/licenses/gpl-2.0.html.

#ifndef __AUTHENTICATION_PROVIDER_H__
#define __AUTHENTICATION_PROVIDER_H__

#ifdef __cplusplus
extern "C" {
#endif

// TODO - Limits here are based on PgSQL
#define LARGE_REGISTRY_LEN          4096
#define MEDIUM_REGISTRY_LEN         1024
#define SMALL_REGISTRY_LEN          10	
#define MEDIUM_SMALL_REGISTRY_LEN   16

#define FOREACH_AUTHTYPE(AUTHTYPE) \
    AUTHTYPE(ADFS)      \
    AUTHTYPE(IAM)       \
    AUTHTYPE(OKTA)      \
    AUTHTYPE(INVALID)   \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum {
    FOREACH_AUTHTYPE(GENERATE_ENUM)
} FederatedAuthType;

static const char *FEDERATEDAUTHTYPE_STRING[] = {
    FOREACH_AUTHTYPE(GENERATE_STRING)
};

typedef struct {
    char idp_endpoint[MEDIUM_REGISTRY_LEN];
    char idp_port[SMALL_REGISTRY_LEN];
    char relaying_party_id[MEDIUM_REGISTRY_LEN];
    char iam_role_arn[MEDIUM_REGISTRY_LEN];
    char iam_idp_arn[MEDIUM_REGISTRY_LEN];
    char idp_username[MEDIUM_REGISTRY_LEN];
    char idp_password[MEDIUM_REGISTRY_LEN];
    char http_client_socket_timeout[SMALL_REGISTRY_LEN];
    char http_client_connect_timeout[SMALL_REGISTRY_LEN];
    char ssl_insecure[SMALL_REGISTRY_LEN];
} FederatedAuthConfig;

typedef struct Credentials {
    char *username;
    char *password;
    unsigned int username_size;
    unsigned int password_size;
} Credentials;

FederatedAuthType GetFedAuthTypeEnum(const char *str);

bool GetCachedToken(char* token, const unsigned int maxSize, const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName);
void UpdateCachedToken(const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName, const char* token, const char* expirationTime);
bool GenerateConnectAuthToken(char* token, const unsigned int maxSize, const char* dbHostName, const char* dbRegion, unsigned port, const char* dbUserName, FederatedAuthType type, FederatedAuthConfig config);

bool GetCredentialsFromSecretsManager(const char *secretId, const char *region, Credentials *credentials);

#ifdef __cplusplus
}
#endif

#endif  //__AUTHENTICATION_PROVIDER_H__
