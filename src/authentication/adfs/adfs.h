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

#ifndef __ADFS_H__
#define __ADFS_H__

#include "../authentication_provider.h"
#include "../federation.h"

#include <map>
#include <string>

class AdfsCredentialsProvider : public FederationCredentialProvider {
public:
    AdfsCredentialsProvider(
        const FederatedAuthConfig& config,
        std::shared_ptr<Aws::Http::HttpClient> http_client,
        std::shared_ptr<Aws::STS::STSClient> sts_client
    ): FederationCredentialProvider(config.iam_idp_arn, config.iam_role_arn, http_client, sts_client), cfg(config) {}

    // constant pattern strings 
    static const std::string FORM_ACTION_PATTERN;
    static const std::string SAML_RESPONSE_PATTERN;
    static const std::string URL_PATTERN;
    static const std::string INPUT_TAG_PATTERN;

protected:
    virtual std::string GetSAMLAssertion(std::string& err_info);

private:
    std::string get_signin_url();
    std::map<std::string, std::string> get_para_from_html_body(std::string& body);
    std::string get_form_action_body(std::string& url, std::map<std::string, std::string>& params);

    static bool validate_url(const std::string& url);
    static std::string escape_html_entity(const std::string& html);
    static std::vector<std::string> get_input_tags_from_html(const std::string& body);
    static std::string get_value_by_key(const std::string& input, const std::string& key);

    FederatedAuthConfig cfg;
};

#endif  //__ADFS_H__
