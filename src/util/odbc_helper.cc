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

#include "odbc_helper.h"
#include "logger_wrapper.h"
#include "text_helper.h"

SQLTCHAR *OdbcHelper::check_connection_query = const_cast<SQLTCHAR *>(reinterpret_cast<const SQLTCHAR *>(TEXT("SELECT 1")));
SQLTCHAR *OdbcHelper::check_limitless_cluster_query =
    const_cast<SQLTCHAR *>(reinterpret_cast<const SQLTCHAR *>(TEXT(\
        "SELECT EXISTS ("\
        "   SELECT 1"\
        "   FROM pg_catalog.pg_class c"\
        "   JOIN pg_catalog.pg_namespace n ON c.relnamespace = n.oid"\
        "   WHERE c.relname = 'limitless_subclusters'"\
        "   AND n.nspname = 'rds_aurora'"\
        ");"\
    )));

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

bool OdbcHelper::CheckLimitlessCluster(SQLHDBC conn) {
    LoggerWrapper::initialize();

    HSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
    if (!SQL_SUCCEEDED(rc)) {
        return false;
    }

    rc = SQLExecDirect(hstmt, check_limitless_cluster_query, SQL_NTS);
    if (!SQL_SUCCEEDED(rc)) {
        CheckResult(rc, "SQLExecDirect failed", hstmt, SQL_HANDLE_STMT);
        Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, hstmt);
        return false;
    }

    rc = SQLFetch(hstmt);
    if (!SQL_SUCCEEDED(rc)) {
        CheckResult(rc, "SQLFetch failed", hstmt, SQL_HANDLE_STMT);
        Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, hstmt);
        return false;
    }

    SQLTCHAR result[2];
    SQLLEN result_len = 0;
    rc = SQLGetData(hstmt, 1, SQL_C_CHAR, &result, sizeof(result), &result_len);
    if (SQL_SUCCEEDED(rc)) {
        Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, hstmt);
        return result[0] == static_cast<SQLTCHAR>(TEXT('1'));
    }

    CheckResult(rc, "SQLGetData failed", hstmt, SQL_HANDLE_STMT);
    Cleanup(SQL_NULL_HANDLE, SQL_NULL_HANDLE, hstmt);
    return false;
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
#ifdef UNICODE
            // TODO: Add logging of Unicode message
#else
            LOG(ERROR) << sqlstate << message;
#endif            
        }
            
    } while (ret == SQL_SUCCESS);

    if (ret == SQL_NO_DATA && recno == 1) {
        LOG(ERROR) << "No error information";
    }
}
