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

#define CHECK_LIMITLESS_CLUSTER_QUERY\
    "SELECT EXISTS ("\
    "   SELECT 1"\
    "   FROM pg_catalog.pg_class c"\
    "   JOIN pg_catalog.pg_namespace n ON c.relnamespace = n.oid"\
    "   WHERE c.relname = 'limitless_subclusters'"\
    "   AND n.nspname = 'rds_aurora'"\
    ");"

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

    rc = SQLExecDirect(hstmt, const_cast<SQLCHAR *>(reinterpret_cast<const SQLCHAR *>("SELECT 1")), SQL_NTS);
    SQLFreeStmt(hstmt, SQL_CLOSE);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    return SQL_SUCCEEDED(rc);
}

bool OdbcHelper::CheckLimitlessCluster(SQLHDBC conn) {
    HSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
    if (!SQL_SUCCEEDED(rc)) {
        return false;
    }

    rc = SQLExecDirect(hstmt, const_cast<SQLCHAR *>(reinterpret_cast<const SQLCHAR *>(CHECK_LIMITLESS_CLUSTER_QUERY)), SQL_NTS);
    SQLFreeStmt(hstmt, SQL_CLOSE);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    if (SQL_SUCCEEDED(rc)) {
        SQLINTEGER result;
        SQLLEN len;
        rc = SQLGetData(hstmt, 1, SQL_C_SLONG, &result, sizeof(result), &len);
        return SQL_SUCCEEDED(rc) && result == 1;
    }

    return false;
}

void OdbcHelper::LogMessage(const std::string& log_message, SQLHANDLE handle, int32_t handle_type) {
    SQLCHAR     sqlstate[MAX_STATE_LENGTH];
    SQLCHAR     message[MAX_MSG_LENGTH];
    SQLINTEGER	nativeerror;
    SQLSMALLINT textlen;
    SQLRETURN	ret;
    SQLSMALLINT	recno = 0;

    LOG(ERROR) << log_message;

    do {
        recno++;
        ret = SQLGetDiagRec(handle_type, handle, recno, sqlstate, &nativeerror,
                            message, sizeof(message), &textlen);
        if (ret == SQL_INVALID_HANDLE)
            LOG(ERROR) << "Invalid handle";
        else if (SQL_SUCCEEDED(ret))
            LOG(ERROR) << sqlstate << message;
    } while (ret == SQL_SUCCESS);

    if (ret == SQL_NO_DATA && recno == 1) {
        LOG(ERROR) << "No error information";
    }
}
