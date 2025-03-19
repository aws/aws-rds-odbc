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

#ifndef ODBCHELPER_H_
#define ODBCHELPER_H_

#ifdef WIN32
    #include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <string>

#include "string_helper.h"
#include "text_helper.h"

class OdbcHelper {
public:
    static const int MAX_STATE_LENGTH = 32;
    static const int MAX_MSG_LENGTH = 1024;
    static SQLTCHAR *check_connection_query;

    static bool ConnStrConnect(SQLTCHAR* conn_str, SQLHDBC& out_conn);
    static bool CheckResult(SQLRETURN rc, const std::string& log_message, SQLHANDLE handle, int32_t handle_type);
    static bool CheckConnection(SQLHDBC hdbc);
    static void Cleanup(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt);
    static bool AllocateHandle(SQLSMALLINT handle_type, SQLHANDLE input_handle, SQLHANDLE& output_handle, const std::string& log_message);
    static bool ExecuteQuery(SQLHSTMT stmt, SQLTCHAR* query, const std::string& log_message);
    static bool BindColumn(SQLHSTMT stmt, SQLUSMALLINT col_num, SQLSMALLINT type, SQLPOINTER dest, SQLLEN dest_size, const std::string& log_message);
    static bool FetchResults(SQLHSTMT stmt, const std::string& log_message);

private:
    static void LogMessage(const std::string& log_message, SQLHANDLE handle, int32_t handle_type);     
};

class IOdbcHelper {
public:
    virtual bool ConnStrConnect(SQLTCHAR* conn_str, SQLHDBC& out_conn) = 0;
    virtual bool CheckResult(SQLRETURN rc, const std::string& log_message, SQLHANDLE handle, int32_t handle_type) = 0;
    virtual bool CheckConnection(SQLHDBC hdbc) = 0;
    virtual void Cleanup(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt) = 0;
    virtual bool AllocateHandle(SQLSMALLINT handle_type, SQLHANDLE input_handle, SQLHANDLE& output_handle, const std::string& log_message) = 0;
    virtual bool ExecuteQuery(SQLHSTMT stmt, SQLTCHAR* query, const std::string& log_message) = 0;
    virtual bool BindColumn(SQLHSTMT stmt, SQLUSMALLINT col_num, SQLSMALLINT type, SQLPOINTER dest, SQLLEN dest_size, const std::string& log_message) = 0;
    virtual bool FetchResults(SQLHSTMT stmt, const std::string& log_message) = 0;
};

class OdbcHelperWrapper : public IOdbcHelper {
public:
    bool ConnStrConnect(SQLTCHAR* conn_str, SQLHDBC& out_conn) override { return OdbcHelper::ConnStrConnect(conn_str, out_conn); };
    bool CheckResult(SQLRETURN rc, const std::string& log_message, SQLHANDLE handle, int32_t handle_type) override { return OdbcHelper::CheckResult(rc, log_message, handle, handle_type); };
    bool CheckConnection(SQLHDBC hdbc) override { return OdbcHelper::CheckConnection(hdbc); };
    void Cleanup(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt) override { OdbcHelper::Cleanup(henv, hdbc, hstmt); };
    bool AllocateHandle(SQLSMALLINT handle_type, SQLHANDLE input_handle, SQLHANDLE& output_handle, const std::string& log_message) override { return OdbcHelper::AllocateHandle(handle_type, input_handle, output_handle, log_message); };
    bool ExecuteQuery(SQLHSTMT stmt, SQLTCHAR* query, const std::string& log_message) override { return OdbcHelper::ExecuteQuery(stmt, query, log_message); };
    bool BindColumn(SQLHSTMT stmt, SQLUSMALLINT col_num, SQLSMALLINT type, SQLPOINTER dest, SQLLEN dest_size, const std::string& log_message) override { return OdbcHelper::BindColumn(stmt, col_num, type, dest, dest_size, log_message); };
    bool FetchResults(SQLHSTMT stmt, const std::string& log_message) override { return OdbcHelper::FetchResults(stmt, log_message); };
};

#endif // ODBCHELPER_H_
