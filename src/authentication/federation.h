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

#ifndef FEDERATION_H_
#define FEDERATION_H_

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/http/HttpClient.h>
#include <aws/sts/STSClient.h>
#include <aws/sts/model/AssumeRoleWithSAMLRequest.h>

class FederationCredentialProvider {
public:
    FederationCredentialProvider(std::string idp_arn, std::string role_arn,
        std::shared_ptr<Aws::Http::HttpClient> http_client, std::shared_ptr<Aws::STS::STSClient> sts_client)
        : idp_arn(std::move(idp_arn)), role_arn(std::move(role_arn)),
        http_client(std::move(http_client)), sts_client(std::move(sts_client)) {}

    bool GetAWSCredentials(Aws::Auth::AWSCredentials& credentials);

protected:
    virtual std::string GetSAMLAssertion(std::string& err_info) = 0;

    bool FetchCredentialsWithSAMLAssertion(Aws::STS::Model::AssumeRoleWithSAMLRequest& saml_req,
        Aws::Auth::AWSCredentials& credentials);

    std::string idp_arn;
    std::string role_arn;
    std::shared_ptr<Aws::Http::HttpClient> http_client;
    std::shared_ptr<Aws::STS::STSClient> sts_client;
};

#endif // FEDERATION_H_
