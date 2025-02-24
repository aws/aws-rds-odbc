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

#include "okta.h"

#include <regex>
#include <unordered_set>

#include "../html_util.h"
#include "../util/logger_wrapper.h"

const std::string OktaCredentialsProvider::SAML_RESPONSE_PATTERN = "<input name=\"SAMLResponse\".+value=\"(.+)\"/\\>";

std::string OktaCredentialsProvider::GetSAMLAssertion(std::string& err_info) {
    // SAML Assertion
    std::string url = get_signin_page_url();
    LOG(INFO) << "OKTA Sign In URL w/o Session Token: " << url;
    std::string session_token = get_session_token();
    if (session_token.empty()) {
        LOG(WARNING) << "No session token generated for SAML request";
        return "";
    }
    url += session_token;

    std::shared_ptr<Aws::Http::HttpRequest> req = Aws::Http::CreateHttpRequest(
        url, Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    std::shared_ptr<Aws::Http::HttpResponse> response = http_client->MakeRequest(req);

    std::string retval;
    // check response code
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK) {
        LOG(WARNING) << "OKTA request returned bad HTTP response code: " << response->GetResponseCode();
        err_info = "Okta signOnPageRequest failed.";
        if (response->HasClientError()) {
            err_info += "Client error: '" + response->GetClientErrorMessage() + "'.";
        }
        return retval;
    }

    std::istreambuf_iterator<char> eos;
    std::string body(std::istreambuf_iterator<char>(response->GetResponseBody().rdbuf()), eos);
    DLOG(INFO) << "Signout response body: " << body;

    std::smatch matches;
    if (std::regex_search(body, matches, std::regex(SAML_RESPONSE_PATTERN))) {
        return HtmlUtil::escape_html_entity(matches.str(1));
    }
    LOG(WARNING) << "No SAML response found in response";
    return "";
}

std::string OktaCredentialsProvider::get_session_token() {
    // Send request for session token
    std::string url = get_session_token_url();
    LOG(INFO) << "Got OKTA Session Token URL: " << url;
    std::shared_ptr<Aws::Http::HttpRequest> req = Aws::Http::CreateHttpRequest(
        url, Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    Aws::Utils::Json::JsonValue json_body;
    json_body.WithString("username", std::string(cfg.idp_username))
            .WithString("password", std::string(cfg.idp_password));
    Aws::String json_str = json_body.View().WriteReadable();
    Aws::String json_len = Aws::Utils::StringUtils::to_string(json_str.size());
    req->SetContentType("application/json");
    req->AddContentBody(Aws::MakeShared<Aws::StringStream>("", json_str));
    req->SetContentLength(json_len);
    std::shared_ptr<Aws::Http::HttpResponse> response = http_client->MakeRequest(req);

    // Check resp status
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK) {
        LOG(WARNING) << "OKTA request returned bad HTTP response code: " << response->GetResponseCode();
        if (response->HasClientError()) {
            LOG(WARNING) << "HTTP Client Error: " << response->GetClientErrorMessage();
        }
        return "";
    }

    // Get response session token
    Aws::Utils::Json::JsonValue json_val(response->GetResponseBody());
    if (!json_val.WasParseSuccessful()) {
        LOG(WARNING) << "Unable to parse JSON from response";
        return "";
    }
    Aws::Utils::Json::JsonView json_view = json_val.View();
    if (!json_view.KeyExists("sessionToken")) {
        LOG(WARNING) << "Could not find session token in JSON";
        return ""; 
    }
    return json_view.GetString("sessionToken");
}

std::string OktaCredentialsProvider::get_session_token_url() {
    std::string retval("https://");
    retval += std::string(cfg.idp_endpoint);
    retval += ":";
    retval += std::string(cfg.idp_port);
    retval += "/api/v1/authn";
    return retval;
}

std::string OktaCredentialsProvider::get_signin_page_url() {
    std::string retval("https://");
    retval += std::string(cfg.idp_endpoint);
    retval += ":";
    retval += std::string(cfg.idp_port);
    retval += "/app/amazon_aws/";
    retval += std::string(cfg.app_id);
    retval += "/sso/saml";
    retval += "?onetimetoken=";
    return retval;
}
