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
