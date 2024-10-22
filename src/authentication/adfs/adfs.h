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
        std::shared_ptr<Aws::Http::HttpClient> httpClient,
        std::shared_ptr<Aws::STS::STSClient> stsClient
    ): FederationCredentialProvider(config.iam_idp_arn, config.iam_role_arn, httpClient, stsClient), cfg(config) {}

    // constant pattern strings 
    static const std::string FORM_ACTION_PATTERN;
    static const std::string SAML_RESPONSE_PATTERN;
    static const std::string URL_PATTERN;
    static const std::string INPUT_TAG_PATTERN;

protected:
    virtual std::string GetSAMLAssertion(std::string& errInfo);

private:
    std::string getSignInPageUrl();
    bool validateUrl(const std::string& url);
    std::string escapeHtmlEntity(const std::string& html);
    std::vector<std::string> getInputTagsFromHTML(const std::string& body);
    std::string getValueByKey(const std::string& input, const std::string& key);
    std::map<std::string, std::string> getParametersFromHtmlBody(std::string& body);
    std::string getFormActionBody(std::string& url, std::map<std::string, std::string>& params);

    FederatedAuthConfig cfg;
};

#endif  //__ADFS_H__
