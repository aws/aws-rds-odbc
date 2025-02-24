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

#ifndef ADFS_H_
#define ADFS_H_

#include <map>
#include <string>

#include "../authentication_provider.h"
#include "../federation.h"

class AdfsCredentialsProvider : public FederationCredentialProvider {
public:
    AdfsCredentialsProvider(
        const FederatedAuthConfig& config,
        std::shared_ptr<Aws::Http::HttpClient> http_client,
        std::shared_ptr<Aws::STS::STSClient> sts_client
    ): FederationCredentialProvider(
            config.iam_idp_arn, config.iam_role_arn, std::move(http_client), std::move(sts_client)
        ), cfg(config) {}

    // constant pattern strings 
    static const std::string FORM_ACTION_PATTERN;
    static const std::string INPUT_TAG_PATTERN;
    static const std::string SAML_RESPONSE_PATTERN;
    static const std::string URL_PATTERN;

protected:
    std::string GetSAMLAssertion(std::string& err_info) override;

private:
    std::string get_signin_url();
    std::map<std::string, std::string> get_para_from_html_body(std::string& body);
    std::string get_form_action_body(std::string& url, std::map<std::string, std::string>& params);

    static bool validate_url(const std::string& url);
    static std::vector<std::string> get_input_tags_from_html(const std::string& body);
    static std::string get_value_by_key(const std::string& input, const std::string& key);

    FederatedAuthConfig cfg;
};

#endif // ADFS_H_
