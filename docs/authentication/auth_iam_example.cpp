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

// Include required header
#include <authentication/authentication_provider.h>

#include <stdio.h>
#include <stdlib.h>

#define MAX_TOKEN_SIZE 1024

int main(int argc, char* argv[]) {
    // Specify authentication type [IAM, ADFS, OKTA]
    FederatedAuthType authType = FederatedAuthType::IAM;

    // Specify authentication configurations if any
    FederatedAuthConfig authConfig;

    // Allocate memory to store the token
    char new_token[MAX_TOKEN_SIZE];

    // Specify DB properties
    char* db_server = "sample-db.cluster-abc.us-west-2.rds.amazonaws.com";
    char* region = "us-west-2";
    char* db_port = "5432";
    char* iam_db_username = "jane_doe";

    // This will generate a new token replace the value of your new token
    GenerateConnectAuthToken(new_token, MAX_TOKEN_SIZE,
        db_server, region, atoi(db_port), iam_db_username,
        authType, authConfig);

    // Can also cache and store the value...
    char* token_expiration = "900";
    UpdateCachedToken(db_server, region, db_port, iam_db_username, new_token, token_expiration);

    // ... and retrieve the value from the cache
    GetCachedToken(new_token, MAX_TOKEN_SIZE, db_server, region, db_port, iam_db_username);

    // ... do things with credentials...
}
