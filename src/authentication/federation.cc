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
