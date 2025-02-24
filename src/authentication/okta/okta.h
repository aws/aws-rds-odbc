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

#ifndef OKTA_H_
#define OKTA_H_

#include <map>
#include <string>

#include "../authentication_provider.h"
#include "../federation.h"

class OktaCredentialsProvider : public FederationCredentialProvider {
public:
    OktaCredentialsProvider(
        const FederatedAuthConfig& config,
        std::shared_ptr<Aws::Http::HttpClient> http_client,
        std::shared_ptr<Aws::STS::STSClient> sts_client
    ): FederationCredentialProvider(
            config.iam_idp_arn, config.iam_role_arn, std::move(http_client), std::move(sts_client)
        ), cfg(config) {}

    // constant pattern strings 
    static const std::string SAML_RESPONSE_PATTERN;

protected:
    std::string GetSAMLAssertion(std::string& err_info) override;

private:
    std::string get_session_token();
    std::string get_session_token_url();
    std::string get_signin_page_url();

    FederatedAuthConfig cfg;
};

#endif // OKTA_H_
