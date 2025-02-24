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
#include "logger_wrapper.h"

SQLTCHAR *OdbcHelper::check_connection_query = AS_SQLTCHAR(TEXT("SELECT 1"));

bool OdbcHelper::CheckResult(SQLRETURN rc, const std::string& log_message, SQLHANDLE handle, int32_t handle_type) {
    if (SQL_SUCCEEDED(rc)) {
        return true;
    }

    if (!log_message.empty()) {
        LogMessage(log_message, handle, handle_type);
    }
    
    return false;
}

bool OdbcHelper::CheckConnection(SQLHDBC conn) {
    HSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
    if (!SQL_SUCCEEDED(rc)) {
        return false;
    }

    rc = SQLExecDirect(hstmt, check_connection_query, SQL_NTS);
    SQLFreeStmt(hstmt, SQL_CLOSE);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    return SQL_SUCCEEDED(rc);
}

void OdbcHelper::Cleanup(SQLHENV henv, SQLHDBC conn, SQLHSTMT hstmt) {
    if (hstmt != SQL_NULL_HANDLE) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
    if (conn != SQL_NULL_HANDLE) {
        SQLDisconnect(conn);
        SQLFreeHandle(SQL_HANDLE_DBC, conn);
    }
    if (henv != SQL_NULL_HANDLE) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
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

bool OdbcHelper::ExecuteQuery(SQLHSTMT stmt, SQLTCHAR* query, const std::string& log_message) {
    SQLRETURN rc;
    rc = SQLExecDirect(stmt, query, SQL_NTS);

    if (!OdbcHelper::CheckResult(rc, log_message, stmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, stmt);
        return false;
    };
    return true;
}

bool OdbcHelper::BindColumn(SQLHSTMT stmt, SQLUSMALLINT col_num, SQLSMALLINT type, SQLPOINTER dest, SQLLEN dest_size, const std::string& log_message) {
    SQLRETURN rc;
    rc = SQLBindCol(stmt, col_num, type, dest, dest_size, nullptr);
    if (!OdbcHelper::CheckResult(rc, log_message, stmt, SQL_HANDLE_STMT)) {
        Cleanup(nullptr, nullptr, stmt);
        return false;
    };
    return true;
}

bool OdbcHelper::FetchResults(SQLHSTMT stmt, const std::string& log_message) {
    SQLRETURN rc;
    rc = SQLFetch(stmt);
    if (!OdbcHelper::CheckResult(rc, log_message, stmt, SQL_HANDLE_STMT)) {
        Cleanup(nullptr, nullptr, stmt);
        return false;
    }
    return true;    
}

void OdbcHelper::LogMessage(const std::string& log_message, SQLHANDLE handle, int32_t handle_type) {
    SQLTCHAR     sqlstate[MAX_STATE_LENGTH];
    SQLTCHAR     message[MAX_MSG_LENGTH];
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
            #ifdef UNICODE
            LOG(ERROR) << StringHelper::to_string(sqlstate) << ": " << StringHelper::to_string(message);
            #else
            LOG(ERROR) << sqlstate << ": " << message;
            #endif            
        }

    } while (ret == SQL_SUCCESS);

    if (ret == SQL_NO_DATA && recno == 1) {
        LOG(ERROR) << "No error information";
    }
}
