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

#include "federation.h"
#include "../util/logger_wrapper.h"

bool FederationCredentialProvider::FetchCredentialsWithSAMLAssertion(
    Aws::STS::Model::AssumeRoleWithSAMLRequest& saml_request,
    Aws::Auth::AWSCredentials& credentials) {

    Aws::STS::Model::AssumeRoleWithSAMLOutcome outcome = sts_client->AssumeRoleWithSAML(saml_request);

    bool retval = false;
    if (outcome.IsSuccess()) {
        const Aws::STS::Model::Credentials& new_cred = outcome.GetResult().GetCredentials();

        LOG(INFO) << "Access key is " << new_cred.GetAccessKeyId().c_str() << ", secret key length is " << new_cred.GetSecretAccessKey().size();

        credentials.SetAWSAccessKeyId(new_cred.GetAccessKeyId());
        credentials.SetAWSSecretKey(new_cred.GetSecretAccessKey());
        credentials.SetSessionToken(new_cred.GetSessionToken());

        retval = true;
    } else {
        const auto& error = outcome.GetError();
        std::string err_info = "Failed to fetch credentials, ERROR: " + error.GetExceptionName()
            + ": " + error.GetMessage();
        LOG(ERROR) << "Error in FetchCredentialsWithSAMLAssertion is " << err_info.c_str();
    }

    return retval;
}

bool FederationCredentialProvider::GetAWSCredentials(Aws::Auth::AWSCredentials& credentials) {
    std::string err_info;
    std::string saml_assertion = GetSAMLAssertion(err_info);

    bool retval = false;
    if (saml_assertion.empty()) {
        LOG(ERROR) << "Error in GetAWSCredentials is " << err_info.c_str();
    } else {
        DLOG(INFO) << "SAML assertion is " << saml_assertion.c_str();

        Aws::STS::Model::AssumeRoleWithSAMLRequest saml_request;
        saml_request.WithRoleArn(role_arn)
            .WithSAMLAssertion(saml_assertion)
            .WithPrincipalArn(idp_arn);

        retval = FetchCredentialsWithSAMLAssertion(saml_request, credentials);
    }

    return retval;
}
