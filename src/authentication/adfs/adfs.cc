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
#include "../html_util.h"

#ifndef XCODE_BUILD
#include "../util/logger_wrapper.h"
#endif

#include <regex>
#include <unordered_set>

const std::string AdfsCredentialsProvider::FORM_ACTION_PATTERN = "<form.*?action=\"([^\"]+)\"";
const std::string AdfsCredentialsProvider::SAML_RESPONSE_PATTERN = "SAMLResponse\\W+value=\"(.*)\"( />)";
const std::string AdfsCredentialsProvider::URL_PATTERN = "^(https)://[-a-zA-Z0-9+&@#/%?=~_!:,.']*[-a-zA-Z0-9+&@#/%=~_']";
const std::string AdfsCredentialsProvider::INPUT_TAG_PATTERN = "<input id=(.*)";

std::string AdfsCredentialsProvider::GetSAMLAssertion(std::string& err_info) {
    std::string url = get_signin_url();
    #ifndef XCODE_BUILD
    LOG(INFO) << "Got ADFS URL: " << url;
    #endif

    std::shared_ptr<Aws::Http::HttpRequest> req = Aws::Http::CreateHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    std::shared_ptr<Aws::Http::HttpResponse> response = http_client->MakeRequest(req);

    std::string retval;
    // check response code
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK) {
        #ifndef XCODE_BUILD
        LOG(WARNING) << "ADFS request returned bad HTTP response code: " << response->GetResponseCode();
        #endif
        err_info = "Adfs signOnPageRequest failed.";
        if (response->HasClientError()) {
            err_info += "Client error: '" + response->GetClientErrorMessage() + "'.";
        }
        return retval;
    }

    std::istreambuf_iterator<char> eos;
    std::string body(std::istreambuf_iterator<char>(response->GetResponseBody().rdbuf()), eos);
    #ifndef XCODE_BUILD
    DLOG(INFO) << "Signout response body: " << body;
    #endif

    // retrieve SAMLResponse value
    std::smatch matches;
    std::string action;
    if (std::regex_search(body, matches, std::regex(FORM_ACTION_PATTERN))) {
        action = HtmlUtil::escape_html_entity(matches.str(1));
    } else {
        err_info = "Could not extract action from the response body";
        return retval;
    }

    if (!action.empty() && action[0]=='/') {
        url = "https://";
        url += std::string(cfg.idp_endpoint);
        url += ":";
        url += std::string(cfg.idp_port);
        url += action;
    }
    #ifndef XCODE_BUILD
    DLOG(INFO) << "Updated URL [" << url << "] using Action [" << action << "]";
    #endif

    std::map<std::string, std::string> params = get_para_from_html_body(body);
    std::string content = get_form_action_body(url, params);

    if (std::regex_search(content, matches, std::regex(SAML_RESPONSE_PATTERN))) {
        #ifndef XCODE_BUILD
        DLOG(INFO) << "SAML Response: " << matches.str(1);
        #endif
        return matches.str(1);
    }
    #ifndef XCODE_BUILD
    LOG(WARNING) << "Failed SAML Asesertion";
    #endif
    return retval;
}

std::string AdfsCredentialsProvider::get_signin_url() {
    std::string retval("https://");
    retval += std::string(cfg.idp_endpoint);
    retval += ":";
    retval += std::string(cfg.idp_port);
    retval += "/adfs/ls/IdpInitiatedSignOn.aspx?loginToRp=";
    retval += std::string(cfg.relaying_party_id);
    return retval;
}

bool AdfsCredentialsProvider::validate_url(const std::string& url) {
    std::regex pattern(URL_PATTERN);

    if (!regex_match(url, pattern)) {
        #ifndef XCODE_BUILD
        LOG(WARNING) << "Invalid URL, failed to match ADFS URL pattern";
        #endif
        return false;
    }
    return true;
}

std::vector<std::string> AdfsCredentialsProvider::get_input_tags_from_html(const std::string& body) {
    std::unordered_set<std::string> hash_set;
    std::vector<std::string> retval;

    std::smatch matches;
    std::regex pattern(INPUT_TAG_PATTERN);
    std::string source = body;
    while (std::regex_search(source,matches,pattern)) {
        std::string tag = matches.str(0);
        std::string tag_name = get_value_by_key(tag, std::string("name"));
        #ifndef XCODE_BUILD
        DLOG(INFO) << "Tag [" << tag << "], Tag Name [" << tag_name << "]";
        #endif
        std::transform(tag_name.begin(), tag_name.end(), tag_name.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        if (!tag_name.empty() && hash_set.find(tag_name) == hash_set.end()) {
            hash_set.insert(tag_name);
            retval.push_back(tag);
            #ifndef XCODE_BUILD
            DLOG(INFO) << "Saved input_tag: " << tag;
            #endif
        }

        source = matches.suffix().str();
    }

    #ifndef XCODE_BUILD
	DLOG(INFO) << "Input tags vector size: " << retval.size();
    #endif
    return retval;
}

std::string AdfsCredentialsProvider::get_value_by_key(const std::string& input, const std::string& key) {
    std::string pattern("(");
    pattern += key;
    pattern += ")\\s*=\\s*\"(.*?)\"";

    std::smatch matches;
    if (std::regex_search(input, matches, std::regex(pattern))) {
        return HtmlUtil::escape_html_entity(matches.str(2));
    }
    return "";
}

std::map<std::string, std::string> AdfsCredentialsProvider::get_para_from_html_body(std::string& body) {
    std::map<std::string, std::string> parameters;
    for (auto& input_tag : get_input_tags_from_html(body)) {
        std::string name = get_value_by_key(input_tag, std::string("name"));
        std::string value = get_value_by_key(input_tag, std::string("value"));
        #ifndef XCODE_BUILD
        DLOG(INFO) << "Input Tag [" << input_tag << "], Name [" << name << "], Value [" << value << "]";
        #endif
        std::string name_lower(name);
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (name_lower.find("username") != std::string::npos) {
            parameters.insert(std::pair<std::string, std::string>(name, std::string(cfg.idp_username)));
        } else if (name_lower.find("authmethod") != std::string::npos) {
            if (!value.empty()) {
                parameters.insert(std::pair<std::string, std::string>(name, value));
            }
        } else if (name_lower.find("password") != std::string::npos) {
            parameters.insert(std::pair<std::string, std::string>(name, std::string(cfg.idp_password)));
        } else if (!name.empty()) {
            parameters.insert(std::pair<std::string, std::string>(name, value));
        }
    }

    #ifndef XCODE_BUILD
    DLOG(INFO) << "parameters size: " << parameters.size();
    for (auto& itr : parameters) {
        DLOG(INFO) << "Parameter Key [" << itr.first << "], Value Size [" << itr.second.size() << "]";
    }
    #endif

    return parameters;
}

std::string AdfsCredentialsProvider::get_form_action_body(std::string& url, std::map<std::string, std::string>& params) {
    if (!validate_url(url)) {
        return "";
    }

    std::shared_ptr< Aws::Http::HttpRequest > req = Aws::Http::CreateHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    // set content
    std::string body;
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
    std::shared_ptr< Aws::Http::HttpResponse > response = http_client->MakeRequest(req);
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK) {
        #ifndef XCODE_BUILD
        LOG(WARNING) << "ADFS request returned bad HTTP response code: " << response->GetResponseCode();
        #endif
        if (response->HasClientError()) {
            #ifndef XCODE_BUILD
            LOG(WARNING) << "HTTP Client Error: " << response->GetClientErrorMessage();
            #endif
        }
        return "";
    }

    std::istreambuf_iterator< char > eos;
    std::string resp_body(std::istreambuf_iterator< char >(response->GetResponseBody().rdbuf()), eos);
    return resp_body;
}
