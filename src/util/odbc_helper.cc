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

#include "odbc_helper.h"

#include <sqlext.h>

#include <glog/logging.h>

#include "connection_string_keys.h"
#include "connection_string_helper.h"
#include "string_helper.h"

SQLTCHAR *OdbcHelper::check_connection_query = AS_SQLTCHAR(TEXT("SELECT 1"));

bool OdbcHelper::ConnStrConnect(SQLTCHAR* conn_str, SQLHDBC& out_conn) {
    if (SQL_NULL_HANDLE == out_conn) {
        LOG(WARNING) << "Attempted to connect using null HDBC";
        return false;
    }

    SQLRETURN rc = 0;
    rc = SQLDriverConnect(out_conn, nullptr, conn_str, SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);

    return CheckResult(rc, "Failed to connect to host.", out_conn, SQL_HANDLE_DBC);
}

bool OdbcHelper::CheckResult(SQLRETURN rc, const std::string& log_message, SQLHANDLE handle, int32_t handle_type) {
    if (SQL_SUCCEEDED(rc)) {
        // Successfully fetched row.
        return true;
    }

    if (rc == SQL_NO_DATA) {
        // No more data to fetch.
        return false;
    }

    if (!log_message.empty()) {
        LogMessage(log_message, handle, handle_type);
    }
    
    return false;
}

bool OdbcHelper::CheckConnection(SQLHDBC hdbc) {
    if (SQL_NULL_HDBC == hdbc) {
        return false;
    }

    HSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (!SQL_SUCCEEDED(rc)) {
        return false;
    }

    rc = SQLExecDirect(hstmt, check_connection_query, SQL_NTS);
    SQLFreeStmt(hstmt, SQL_CLOSE);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    return SQL_SUCCEEDED(rc);
}

void OdbcHelper::Cleanup(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt) {
    if (SQL_NULL_HANDLE != hstmt) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }
    if (SQL_NULL_HANDLE != hdbc) {
        SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        hdbc = SQL_NULL_HDBC;
    }
    if (SQL_NULL_HANDLE != henv) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        henv = SQL_NULL_HENV;
    }
}

bool OdbcHelper::AllocateHandle(SQLSMALLINT handle_type, SQLHANDLE input_handle, SQLHANDLE& output_handle, const std::string& log_message) {
    SQLRETURN rc;
    rc = SQLAllocHandle(handle_type, input_handle, &output_handle);
    if (!OdbcHelper::CheckResult(rc, log_message, output_handle, handle_type)) {
        switch (handle_type) {
        case SQL_HANDLE_ENV:
            OdbcHelper::Cleanup(output_handle, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
            break;
        case SQL_HANDLE_DBC:
            OdbcHelper::Cleanup(SQL_NULL_HANDLE, output_handle, SQL_NULL_HANDLE);
            break;
        case SQL_HANDLE_STMT:
            OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, output_handle);
            break;
        default:
            break;
        }
        return false;
    }
    return true;
}

bool OdbcHelper::SetHenvToOdbc3(SQLHENV henv, const std::string& log_message) {
    SQLRETURN rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    return OdbcHelper::CheckResult(rc, log_message, henv, SQL_HANDLE_ENV);
}

bool OdbcHelper::ExecuteQuery(SQLHSTMT stmt, SQLTCHAR* query, const std::string& log_message) {
    if (SQL_NULL_HANDLE == stmt) {
        LOG(WARNING) << "Attempted to execute query using null HSTMT";
        return false;
    }

    if (!query) {
        LOG(WARNING) << "Attempted to execute query without a valid query pointer";
        return false;
    }

    SQLRETURN rc;
    rc = SQLExecDirect(stmt, query, SQL_NTS);

    if (!OdbcHelper::CheckResult(rc, log_message, stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return false;
    };
    return true;
}

bool OdbcHelper::BindColumn(SQLHSTMT stmt, SQLUSMALLINT col_num, SQLSMALLINT type, SQLPOINTER dest, SQLLEN dest_size, const std::string& log_message) {
    if (SQL_NULL_HANDLE == stmt) {
        LOG(WARNING) << "Attempted to bind columns to a null HSTMT";
        return false;
    }

    SQLRETURN rc;
    SQLLEN ret_len = 0;
    rc = SQLBindCol(stmt, col_num, type, dest, dest_size, &ret_len);
    if (!OdbcHelper::CheckResult(rc, log_message, stmt, SQL_HANDLE_STMT)) {
        Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return false;
    };
    return true;
}

bool OdbcHelper::FetchResults(SQLHSTMT stmt, const std::string& log_message) {
    if (SQL_NULL_HANDLE == stmt) {
        LOG(WARNING) << "Attempted to fetch results using a null HSTMT";
        return false;
    }

    SQLRETURN rc;
    rc = SQLFetch(stmt);
    if (!OdbcHelper::CheckResult(rc, log_message, stmt, SQL_HANDLE_STMT)) {
        Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return false;
    }
    return true;
}

std::string OdbcHelper::MergeDiagRecs(SQLHANDLE handle, int32_t handle_type, const std::string& custom_errmsg) {
    std::string errmsg;

    SQLTCHAR    sqlstate[MAX_STATE_LENGTH];
    SQLTCHAR    message[MAX_MSG_LENGTH];
    SQLINTEGER  nativeerror;
    SQLSMALLINT textlen;
    SQLRETURN   ret;
    SQLSMALLINT recno = 0;

    // merge all error messages for the failed dbc
    do {
        recno++;
        message[0] = '\0';
        ret = SQLGetDiagRec(handle_type, handle, recno, sqlstate, &nativeerror, message, sizeof(message), &textlen);
        if (SQL_SUCCEEDED(ret)) {
            std::string newmsg = StringHelper::ToString(message);
            errmsg = StringHelper::MergeStrings(errmsg, newmsg);
        }
    } while (ret == SQL_SUCCESS);

    if (custom_errmsg.empty()) {
        return errmsg;
    }

    return StringHelper::MergeStrings(errmsg, custom_errmsg);
}

bool OdbcHelper::TestConnectionToServer(const SQLSTR &in_conn_str, const std::string &server) {
    SQLHENV henv = nullptr;
    SQLHDBC conn = nullptr;

    // build new connection string, overwriting server to provided server
    std::map<SQLSTR, SQLSTR> connstr_map;
    ConnectionStringHelper::ParseConnectionString(in_conn_str, connstr_map);
    connstr_map[SERVER_HOST_KEY] = StringHelper::ToSQLSTR(server);
    SQLSTR connstr = ConnectionStringHelper::BuildConnectionString(connstr_map);

    // allocate environment and connection handles
    if (!OdbcHelper::AllocateHandle(SQL_HANDLE_ENV, nullptr, henv, "Couldn't allocate environment handle in LimitlessRouterMonitor::TestConnectionToHost") ||
        !OdbcHelper::SetHenvToOdbc3(henv, "Couldn't set environment handle's ODBC version to 3.0 in LimitlessRouterMonitor::TestConnectionToHost") ||
        !OdbcHelper::AllocateHandle(SQL_HANDLE_DBC, henv, conn, "Couldn't allocate connection handle in LimitlessRouterMonitor::TestConnectionToHost")) {
        OdbcHelper::Cleanup(henv, conn, nullptr);
        return false;
    }

    // connect and check connection
    bool result = OdbcHelper::ConnStrConnect(AS_SQLTCHAR(connstr.c_str()), conn) && OdbcHelper::CheckConnection(conn);

    OdbcHelper::Cleanup(henv, conn, nullptr);
    return result;
}

void OdbcHelper::LogMessage(const std::string& log_message, SQLHANDLE handle, int32_t handle_type) {
    SQLTCHAR    sqlstate[MAX_STATE_LENGTH];
    SQLTCHAR    message[MAX_MSG_LENGTH];
    SQLINTEGER  nativeerror;
    SQLSMALLINT textlen;
    SQLRETURN   ret;
    SQLSMALLINT recno = 0;

    LOG(ERROR) << log_message;

    do {
        recno++;
        ret = SQLGetDiagRec(handle_type, handle, recno, sqlstate, &nativeerror,
                            message, sizeof(message), &textlen);
        if (ret == SQL_INVALID_HANDLE) {
            LOG(ERROR) << "Invalid handle";
        } else if (SQL_SUCCEEDED(ret)) {
            LOG(ERROR) << StringHelper::ToString(sqlstate) << ": "
                       << StringHelper::ToString(message);
        }

    } while (ret == SQL_SUCCESS);

    if (ret == SQL_NO_DATA && recno == 1) {
        LOG(ERROR) << "No error information";
    }
}
