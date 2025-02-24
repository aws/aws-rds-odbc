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
