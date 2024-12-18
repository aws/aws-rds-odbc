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

#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TOKEN_SIZE 1024

int main(int argc, char* argv[]) {
    // Allocate memory to store the username and password
    Credentials credentials;
    credentials.username = (char*) malloc(MAX_TOKEN_SIZE);
    credentials.password = (char*) malloc(MAX_TOKEN_SIZE);
    credentials.username_size = MAX_TOKEN_SIZE;
    credentials.password_size = MAX_TOKEN_SIZE;

    // Specify Secrets Manager Key
    // This can either full ARN or Secret ID
    char* secret_id = "arn:aws:secretsmanager:us-west-2:123412341234:secret:secret-id";
    // If given a full ARN, the region will automatically be parsed
    // Else, will need to provide the region the secret is stored in
    char* secret_region = "us-west-2";

    GetCredentialsFromSecretsManager(secret_id, secret_region, &credentials);

    // ... do things with credentials...

    // Cleanup
    free(credentials.username);
    free(credentials.password);
}
