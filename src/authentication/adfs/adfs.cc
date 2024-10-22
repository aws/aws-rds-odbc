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

#include "adfs.h"

#include "../util/logger_wrapper.h"

#include <regex>
#include <unordered_set>

const std::string AdfsCredentialsProvider::FORM_ACTION_PATTERN = "<form.*?action=\"([^\"]+)\"";
const std::string AdfsCredentialsProvider::SAML_RESPONSE_PATTERN = "SAMLResponse\\W+value=\"(.*)\"( />)";
const std::string AdfsCredentialsProvider::URL_PATTERN = "^(https)://[-a-zA-Z0-9+&@#/%?=~_!:,.']*[-a-zA-Z0-9+&@#/%=~_']";
const std::string AdfsCredentialsProvider::INPUT_TAG_PATTERN = "<input id=(.*)";

std::string AdfsCredentialsProvider::GetSAMLAssertion(std::string& errInfo) {
    std::string url = getSignInPageUrl();
    LOG(INFO) << "Got ADFS URL: " << url;

    std::shared_ptr<Aws::Http::HttpRequest> req = Aws::Http::CreateHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    std::shared_ptr<Aws::Http::HttpResponse> response = httpClient->MakeRequest(req);

    std::string retval("");
    // check response code
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK) {
        LOG(WARNING) << "ADFS request returned bad HTTP response code: " << response->GetResponseCode();
        errInfo = "Adfs signOnPageRequest failed.";
        if (response->HasClientError()) {
            errInfo += "Client error: '" + response->GetClientErrorMessage() + "'.";
        }
        return retval;
    }

    std::istreambuf_iterator<char> eos;
    std::string body(std::istreambuf_iterator<char>(response->GetResponseBody().rdbuf()), eos);
    DLOG(INFO) << "Signout response body: " << body;

    // retrieve SAMLResponse value
    std::smatch matches;
    std::string action("");
    if (std::regex_search(body, matches, std::regex(FORM_ACTION_PATTERN))) {
        action = escapeHtmlEntity(matches.str(1));
    } else {
        errInfo = "Could not extract action from the response body";
        return retval;
    }

    if (!action.empty() && action[0]=='/') {
        url = "https://";
        url += std::string(cfg.idp_endpoint);
        url += ":";
        url += std::string(cfg.idp_port);
        url += action;
    }
    DLOG(INFO) << "Updated URL [" << url << "] using Action [" << action << "]";

    std::map<std::string, std::string> params = getParametersFromHtmlBody(body);
    std::string content = getFormActionBody(url, params);

    if (std::regex_search(content, matches, std::regex(SAML_RESPONSE_PATTERN))) {
        DLOG(INFO) << "SAML Response: " << matches.str(1);
        return matches.str(1);
    } else {
        LOG(WARNING) << "Failed SAML Asesertion";
        return retval;
    }
}

std::string AdfsCredentialsProvider::getSignInPageUrl() {
    std::string retval("https://");
    retval += std::string(cfg.idp_endpoint);
    retval += ":";
    retval += std::string(cfg.idp_port);
    retval += "/adfs/ls/IdpInitiatedSignOn.aspx?loginToRp=";
    retval += std::string(cfg.relaying_party_id);
    return retval;
}

bool AdfsCredentialsProvider::validateUrl(const std::string& url) {
    std::regex pattern(URL_PATTERN);

    if (!regex_match(url, pattern)) {
        LOG(WARNING) << "Invalid URL, failed to match ADFS URL pattern";
        return false; 
    }
    return true;
}

std::string AdfsCredentialsProvider::escapeHtmlEntity(const std::string& html) {
    std::string retval("");
    DLOG(INFO) << "Before HTML escape modification: " << html;
    int i = 0;
    int length = html.length();
    while (i < length) {
        char c = html[i];
        if (c != '&') {
            retval.append(1, c);
            i++;
            continue;
        }

        if (html.substr(i, 4) == "&lt;") {
            retval.append(1,'<');
            i += 4;
        } else if (html.substr(i, 4) == "&gt;") {
            retval.append(1, '>');
            i += 4;
        } else if (html.substr(i, 5) == "&amp;") {
            retval.append(1, '&');
            i += 5;
        } else if (html.substr(i, 6) == "&apos;") {
            retval.append(1, '\'');
            i += 6;
        } else if (html.substr(i, 6) == "&quot;") {
            retval.append(1, '"');
            i += 6;
        } else {
            retval.append(1, c);
            ++i;
        }
    }
    DLOG(INFO) << "After HTML escape modification: " << html;
    return retval;
}

std::vector<std::string> AdfsCredentialsProvider::getInputTagsFromHTML(const std::string& body) {
    std::unordered_set<std::string> hashSet;
    std::vector<std::string> retval;

    std::smatch matches;
    std::regex pattern(INPUT_TAG_PATTERN);
    std::string source = body;
    while (std::regex_search(source,matches,pattern)) {
        std::string tag = matches.str(0);
        std::string tagName = getValueByKey(tag, std::string("name"));
        DLOG(INFO) << "Tag [" << tag << "], Tag Name [" << tagName << "]";
        std::transform(tagName.begin(), tagName.end(), tagName.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        if (!tagName.empty() && hashSet.find(tagName) == hashSet.end()) {
            hashSet.insert(tagName);
            retval.push_back(tag);
            DLOG(INFO) << "Saved inputTag: " << tag;
        }

        source = matches.suffix().str();
    }

	DLOG(INFO) << "Input tags vector size: " << retval.size();
    return retval;
}

std::string AdfsCredentialsProvider::getValueByKey(const std::string& input, const std::string& key) {
    std::string pattern("(");
    pattern += key;
    pattern += ")\\s*=\\s*\"(.*?)\"";

    std::smatch matches;
    if (std::regex_search(input, matches, std::regex(pattern))) {
        return escapeHtmlEntity(matches.str(2));
    } else {
        return "";
    }
}

std::map<std::string, std::string> AdfsCredentialsProvider::getParametersFromHtmlBody(std::string& body) {
    std::map<std::string, std::string> parameters;
    for (auto& inputTag : getInputTagsFromHTML(body)) {
        std::string name = getValueByKey(inputTag, std::string("name"));
        std::string value = getValueByKey(inputTag, std::string("value"));
        DLOG(INFO) << "Input Tag [" << inputTag << "], Name [" << name << "], Value [" << value << "]";
        std::string nameLower = name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (nameLower.find("username") != std::string::npos) {
            parameters.insert(std::pair<std::string, std::string>(name, std::string(cfg.idp_username)));
        } else if (nameLower.find("authmethod") != std::string::npos) {
            if (!value.empty()) {
                parameters.insert(std::pair<std::string, std::string>(name, value));
            }
        } else if (nameLower.find("password") != std::string::npos) {
            parameters.insert(std::pair<std::string, std::string>(name, std::string(cfg.idp_password)));
        } else if (!name.empty()) {
            parameters.insert(std::pair<std::string, std::string>(name, value));
        }
    }

    DLOG(INFO) << "parameters size: " << parameters.size();
    for (auto& itr : parameters) {
        DLOG(INFO) << "Parameter Key [" << itr.first << "], Value Size [" << itr.second.size() << "]";
    }

    return parameters;
}

std::string AdfsCredentialsProvider::getFormActionBody(std::string& url, std::map<std::string, std::string>& params) {
    if (!validateUrl(url)) {
        return "";
    }

    std::shared_ptr< Aws::Http::HttpRequest > req = Aws::Http::CreateHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    // set content
    std::string body("");
    for (auto& itr : params) {
        body += itr.first + "=" + itr.second;
        body += "&";
    }
    body.pop_back();

    std::shared_ptr< Aws::StringStream > ss = std::make_shared< Aws::StringStream >();
    *ss << body;

    req->AddContentBody(ss);
    req->SetContentLength(std::to_string(body.size()));

    // check response code
    std::shared_ptr< Aws::Http::HttpResponse > response = httpClient->MakeRequest(req);
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK) {
        LOG(WARNING) << "ADFS request returned bad HTTP response code: " << response->GetResponseCode();
        if (response->HasClientError()) {
            LOG(WARNING) << "HTTP Client Error: " << response->GetClientErrorMessage();
        }
        return "";
    }

    std::istreambuf_iterator< char > eos;
    std::string rspBody(std::istreambuf_iterator< char >(response->GetResponseBody().rdbuf()), eos);
    return rspBody;
}
