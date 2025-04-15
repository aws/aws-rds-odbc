// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FEDERATION_H_
#define FEDERATION_H_

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/http/HttpClient.h>
#include <aws/sts/STSClient.h>

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
