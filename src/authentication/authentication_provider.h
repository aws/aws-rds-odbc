//  Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef AUTHENTICATION_PROVIDER_H_
#define AUTHENTICATION_PROVIDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

// TODO(yuenhcol) - Limits here are based on PgSQL
#define LARGE_REGISTRY_LEN          4096
#define MEDIUM_REGISTRY_LEN         1024
#define MEDIUM_SMALL_REGISTRY_LEN   16
#define SMALL_REGISTRY_LEN          10

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

static const char* federated_auth_type_str[] = {
    FOREACH_AUTHTYPE(GENERATE_STRING)
};

typedef struct {
    char idp_endpoint[MEDIUM_REGISTRY_LEN];
    char idp_port[SMALL_REGISTRY_LEN];
    char relaying_party_id[MEDIUM_REGISTRY_LEN];
    char app_id[MEDIUM_REGISTRY_LEN];
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

/**
 * Given a string, checks if it is a supported federated authentication type.
 * 
 * @param str a string representing authentication type
 * @return Enum of a valid authentication type
 */
FederatedAuthType GetFedAuthTypeEnum(const char *str);

/**
 * Checks and returns a cached value given the hostname, region, port, and username
 * 
 * @param token an allocated memory space to retrieve the cached token
 * @param max_size the maximum size given to the token
 * @param db_hostname the database hostname
 * @param db_region the database region
 * @param port the database port
 * @param db_user the database username
 * @return True if a cached value was returned in token
 */
bool GetCachedToken(char* token, unsigned int max_size, const char* db_hostname, const char* db_region, const char* port, const char* db_user);

/**
 * Updates the cache for a given hostname, region, port, and username
 * with the new token and expiration time
 * 
 * @param db_hostname the database hostname
 * @param db_region the database region
 * @param port the database port
 * @param db_user the database username
 * @param token an allocated memory space to retrieve the cached token
 * @param expiration_time the maximum size given to the token
 */
void UpdateCachedToken(const char* db_hostname, const char* db_region, const char* port, const char* db_user, const char* token, const char* expiration_time);

/**
 * Given the hostname, region, port, and username, generates a new token for the given authentication type
 * Configuration for the authentication will specify which server to federate against
 * 
 * @param token an allocated memory space to retrieve the cached token
 * @param max_size the maximum size given to the token
 * @param db_hostname the database hostname
 * @param db_region the database region
 * @param port the database port
 * @param db_user the username
 * @param type the enum of the authentication type
 * @param config a struct with 
 * @return True if a cached value was returned in token
 */
bool GenerateConnectAuthToken(char* token, unsigned int max_size, const char* db_hostname, const char* db_region, unsigned port, const char* db_user, FederatedAuthType type, FederatedAuthConfig config);

/**
 * Given a secret ID and region, retrieves secrets for Username and Password from Secrets Manager
 * If the given Secret ID is a full ARN, the region will be parsed from the ARN and user input is ignored
 * If the secret ID is not a full ARN, and the user does not provide a region, region will default to us-east-1
 * 
 * @param secret_id the full ARN or secret ID of a Secret, given a full
 * @param region the region of the Secret ID
 * @param credentials a struct containing allocated memory space to return the username and password from Secrets Manager
 * @return True if credentials were retrieved and updated
 */
bool GetCredentialsFromSecretsManager(const char* secret_id, const char* region, Credentials* credentials);

#ifdef __cplusplus
}
#endif

#endif // AUTHENTICATION_PROVIDER_H_
