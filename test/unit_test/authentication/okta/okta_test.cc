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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../mock_objects.h"

#include <okta/okta.h>

namespace {
    const char* idp_endpoint("endpoint.com");
    const char* idp_port("1234");
    const char* app_id("urn:amazon:webservices");
    const char* iam_role_arn("arn:aws:iam::012345678910:saml-provider/okta");
    const char* iam_idp_arn("arn:aws:iam::012345678910:role/okta_iam_role");
    const char* idp_username("my_user");
    const char* idp_password("my_pass");
    const char* http_client_socket_timeout("1111");
    const char* http_client_connect_timeout("2222");
    const char* ssl_insecure("allow");
    const char* access_key("test_access_key");
    const char* secret_key("test_secret_key");
    const char* session_key("test_session_key");
    std::string resp_token_stream("{\"sessionToken\": \"longuniquesessiontoken\"}");
    std::string resp_saml_stream("<input name=\"SAMLResponse\" type=\"hidden\" value=\"superlongsamlresponsevalue;\"/>");
}

static Aws::SDKOptions sdk_options;

class Oktatest : public testing::Test {
  protected:
    std::shared_ptr<MOCK_HTTP_CLIENT> mock_http_client;
    std::shared_ptr<MOCK_STS_CLIENT> mock_sts_client;

    // Runs once per suite
    static void SetUpTestSuite() {
        Aws::InitAPI(sdk_options);
    }
    static void TearDownTestSuite() {
        Aws::ShutdownAPI(sdk_options);
    }

    // Runs per test case
    void SetUp() override {
        mock_http_client = std::make_shared<MOCK_HTTP_CLIENT>();
        mock_sts_client = std::make_shared<MOCK_STS_CLIENT>();
    }
    void TearDown() override {}

    void GetStdOktaCfg(FederatedAuthConfig& cfg) {
        strcpy(cfg.idp_endpoint, idp_endpoint);
        strcpy(cfg.idp_port, idp_port);
        strcpy(cfg.app_id, app_id);
        strcpy(cfg.iam_role_arn, iam_role_arn);
        strcpy(cfg.iam_idp_arn, iam_idp_arn);
        strcpy(cfg.idp_username, idp_username);
        strcpy(cfg.idp_password, idp_password);
    }
};

TEST_F(Oktatest, GetAWSCredentials_Success) {
    std::shared_ptr<MOCK_HTTP_RESP> resp;
    resp = std::make_shared<MOCK_HTTP_RESP>();
    EXPECT_CALL(*resp, GetResponseCode())
        .WillOnce(testing::Return(Aws::Http::HttpResponseCode::OK));
    std::shared_ptr<Aws::IOStream> resp_body =
        std::make_shared<std::stringstream>(resp_token_stream);
    EXPECT_CALL(*resp, GetResponseBody())
        .WillOnce(testing::ReturnRef(*resp_body));

    std::shared_ptr<MOCK_HTTP_RESP> session_token_resp;
    session_token_resp = std::make_shared<MOCK_HTTP_RESP>();
    EXPECT_CALL(*session_token_resp, GetResponseCode())
        .WillOnce(testing::Return(Aws::Http::HttpResponseCode::OK));
    std::shared_ptr<Aws::IOStream> session_token_body =
        std::make_shared<std::stringstream>(resp_saml_stream);
    EXPECT_CALL(*session_token_resp, GetResponseBody())
        .WillOnce(testing::ReturnRef(*session_token_body));

    EXPECT_CALL(*mock_http_client, MakeRequest(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(resp))
        .WillOnce(testing::Return(session_token_resp));

    std::shared_ptr<Aws::STS::Model::Credentials> expected_credentials;
    expected_credentials = std::make_shared<Aws::STS::Model::Credentials>();
    expected_credentials->SetAccessKeyId(access_key);
    expected_credentials->SetSecretAccessKey(secret_key);
    expected_credentials->SetSessionToken(session_key);
    std::shared_ptr<Aws::STS::Model::AssumeRoleWithSAMLResult> saml_result;    
    saml_result = std::make_shared<Aws::STS::Model::AssumeRoleWithSAMLResult>();
    saml_result->SetCredentials(*expected_credentials);
    Aws::STS::Model::AssumeRoleWithSAMLOutcome saml_outcome(*saml_result);

    EXPECT_CALL(*mock_sts_client, AssumeRoleWithSAML(testing::_))
        .WillOnce(testing::Return(saml_outcome));

    Aws::Auth::AWSCredentials credentials;
    FederatedAuthConfig auth_cfg;
    GetStdOktaCfg(auth_cfg);
    OktaCredentialsProvider okta(auth_cfg, mock_http_client, mock_sts_client);

    EXPECT_TRUE(okta.GetAWSCredentials(credentials));
    EXPECT_STREQ(access_key, credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ(secret_key, credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ(session_key, credentials.GetSessionToken().c_str());
}

TEST_F(Oktatest, GetAWSCredentials_BadRequest_ClientError) {
    std::shared_ptr<MOCK_HTTP_RESP> bad_resp;
    bad_resp = std::make_shared<MOCK_HTTP_RESP>();

    EXPECT_CALL(*bad_resp, GetResponseCode())
        .WillRepeatedly(testing::Return(Aws::Http::HttpResponseCode::NOT_FOUND));
    EXPECT_CALL(*bad_resp, HasClientError())
        .WillRepeatedly(testing::Return(true));
    Aws::String clientErrMsg("Bad Request");
    EXPECT_CALL(*bad_resp, GetClientErrorMessage())
        .WillRepeatedly(testing::ReturnRef(clientErrMsg));

    EXPECT_CALL(*mock_http_client, MakeRequest(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(bad_resp));

    Aws::Auth::AWSCredentials credentials;
    FederatedAuthConfig auth_cfg;
    GetStdOktaCfg(auth_cfg);
    OktaCredentialsProvider okta(auth_cfg, mock_http_client, mock_sts_client);

    EXPECT_FALSE(okta.GetAWSCredentials(credentials));
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}

TEST_F(Oktatest, GetAWSCredentials_BadRequest_SessionTokenResp) {
    std::shared_ptr<MOCK_HTTP_RESP> bad_resp;
    bad_resp = std::make_shared<MOCK_HTTP_RESP>();

    EXPECT_CALL(*bad_resp, GetResponseCode())
        .WillOnce(testing::Return(Aws::Http::HttpResponseCode::OK));
    std::shared_ptr<Aws::IOStream> resp_body =
        std::make_shared<std::stringstream>("Fake Body - Bad Response");
    EXPECT_CALL(*bad_resp, GetResponseBody())
        .WillOnce(testing::ReturnRef(*resp_body));

    EXPECT_CALL(*mock_http_client, MakeRequest(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(bad_resp));

    Aws::Auth::AWSCredentials credentials;
    FederatedAuthConfig auth_cfg;
    GetStdOktaCfg(auth_cfg);
    OktaCredentialsProvider okta(auth_cfg, mock_http_client, mock_sts_client);

    EXPECT_FALSE(okta.GetAWSCredentials(credentials));
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}

TEST_F(Oktatest, GetAWSCredentials_BadSamlResponse) {
    std::shared_ptr<MOCK_HTTP_RESP> resp;
    resp = std::make_shared<MOCK_HTTP_RESP>();
    EXPECT_CALL(*resp, GetResponseCode())
        .WillOnce(testing::Return(Aws::Http::HttpResponseCode::OK));
    std::shared_ptr<Aws::IOStream> resp_body =
        std::make_shared<std::stringstream>(resp_token_stream);
    EXPECT_CALL(*resp, GetResponseBody())
        .WillOnce(testing::ReturnRef(*resp_body));

    std::shared_ptr<MOCK_HTTP_RESP> bad_saml_resp;
    bad_saml_resp = std::make_shared<MOCK_HTTP_RESP>();
    EXPECT_CALL(*bad_saml_resp, GetResponseCode())
        .WillOnce(testing::Return(Aws::Http::HttpResponseCode::OK));
    std::shared_ptr<Aws::IOStream> session_token_body =
        std::make_shared<std::stringstream>("Fake Body - Bad SAML Response");
    EXPECT_CALL(*bad_saml_resp, GetResponseBody())
        .WillOnce(testing::ReturnRef(*session_token_body));

    EXPECT_CALL(*mock_http_client, MakeRequest(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(resp))
        .WillOnce(testing::Return(bad_saml_resp));

    Aws::Auth::AWSCredentials credentials;
    FederatedAuthConfig auth_cfg;
    GetStdOktaCfg(auth_cfg);
    OktaCredentialsProvider okta(auth_cfg, mock_http_client, mock_sts_client);

    EXPECT_FALSE(okta.GetAWSCredentials(credentials));
    EXPECT_STREQ("", credentials.GetAWSAccessKeyId().c_str());
    EXPECT_STREQ("", credentials.GetAWSSecretKey().c_str());
    EXPECT_STREQ("", credentials.GetSessionToken().c_str());
}
