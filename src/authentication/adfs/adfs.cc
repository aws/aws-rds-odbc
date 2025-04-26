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

#include "adfs.h"

#include <regex>
#include <unordered_set>

#include <glog/logging.h>

#include "../html_util.h"
#include "../util/logger_wrapper.h"

const std::string AdfsCredentialsProvider::FORM_ACTION_PATTERN = "<form.*?action=\"([^\"]+)\"";
const std::string AdfsCredentialsProvider::INPUT_TAG_PATTERN = "<input id=(.*)";
const std::string AdfsCredentialsProvider::SAML_RESPONSE_PATTERN = "name=\"SAMLResponse\"\\s+value=\"([^\"]+)\"";
const std::string AdfsCredentialsProvider::URL_PATTERN = "^(https)://[-a-zA-Z0-9+&@#/%?=~_!:,.']*[-a-zA-Z0-9+&@#/%=~_']";

std::string AdfsCredentialsProvider::GetSAMLAssertion(std::string& err_info) {
    std::string url = get_signin_url();
    LOG(INFO) << "Got ADFS URL: " << url;

    std::shared_ptr<Aws::Http::HttpRequest> req = Aws::Http::CreateHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    std::shared_ptr<Aws::Http::HttpResponse> response = http_client->MakeRequest(req);

    std::string retval;
    // check response code
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK) {
        LOG(WARNING) << "ADFS request returned bad HTTP response code: " << response->GetResponseCode();
        err_info = "Adfs signOnPageRequest failed.";
        if (response->HasClientError()) {
            err_info += "Client error: '" + response->GetClientErrorMessage() + "'.";
        }
        return retval;
    }

    std::istreambuf_iterator<char> eos;
    std::string body(std::istreambuf_iterator<char>(response->GetResponseBody().rdbuf()), eos);
    DLOG(INFO) << "Signout response body: " << body;

    // retrieve SAMLResponse value
    std::smatch matches;
    std::string action;
    if (std::regex_search(body, matches, std::regex(FORM_ACTION_PATTERN))) {
        action = HtmlUtil::EscapeHtmlEntity(matches.str(1));
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
    DLOG(INFO) << "Updated URL [" << url << "] using Action [" << action << "]";

    std::map<std::string, std::string> params = get_para_from_html_body(body);
    std::string content = get_form_action_body(url, params);

    if (std::regex_search(content, matches, std::regex(SAML_RESPONSE_PATTERN))) {
        DLOG(INFO) << "SAML Response: " << matches.str(1);
        return matches.str(1);
    }
    LOG(WARNING) << "Failed SAML Asesertion";
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
        LOG(WARNING) << "Invalid URL, failed to match ADFS URL pattern";
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
        DLOG(INFO) << "Tag [" << tag << "], Tag Name [" << tag_name << "]";
        std::transform(tag_name.begin(), tag_name.end(), tag_name.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        if (!tag_name.empty() && !hash_set.contains(tag_name)) {
            hash_set.insert(tag_name);
            retval.push_back(tag);
            DLOG(INFO) << "Saved input_tag: " << tag;
        }

        source = matches.suffix().str();
    }

	DLOG(INFO) << "Input tags vector size: " << retval.size();
    return retval;
}

std::string AdfsCredentialsProvider::get_value_by_key(const std::string& input, const std::string& key) {
    std::string pattern("(");
    pattern += key;
    pattern += ")\\s*=\\s*\"(.*?)\"";

    std::smatch matches;
    if (std::regex_search(input, matches, std::regex(pattern))) {
        return HtmlUtil::EscapeHtmlEntity(matches.str(2));
    }
    return "";
}

std::map<std::string, std::string> AdfsCredentialsProvider::get_para_from_html_body(std::string& body) {
    std::map<std::string, std::string> parameters;
    for (auto& input_tag : get_input_tags_from_html(body)) {
        std::string name = get_value_by_key(input_tag, std::string("name"));
        std::string value = get_value_by_key(input_tag, std::string("value"));
        DLOG(INFO) << "Input Tag [" << input_tag << "], Name [" << name << "], Value [" << value << "]";
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

    DLOG(INFO) << "parameters size: " << parameters.size();
    for (auto& itr : parameters) {
        DLOG(INFO) << "Parameter Key [" << itr.first << "], Value Size [" << itr.second.size() << "]";
    }

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
        LOG(WARNING) << "ADFS request returned bad HTTP response code: " << response->GetResponseCode();
        if (response->HasClientError()) {
            LOG(WARNING) << "HTTP Client Error: " << response->GetClientErrorMessage();
        }
        return "";
    }

    std::istreambuf_iterator< char > eos;
    std::string resp_body(std::istreambuf_iterator< char >(response->GetResponseBody().rdbuf()), eos);
    return resp_body;
}
