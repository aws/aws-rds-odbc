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

#ifndef ODBCHELPER_H_
#define ODBCHELPER_H_

#ifdef WIN32
    #include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <string>

#include "text_helper.h"

#define AS_SQLTCHAR(str) (const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(str)))
#define AS_CHAR(str) (reinterpret_cast<char*>(str))
#define AS_WCHAR(str) (reinterpret_cast<wchar_t*>(str))

class OdbcHelper {
public:
    static const int MAX_STATE_LENGTH = 32;
    static const int MAX_MSG_LENGTH = 1024;
    static SQLTCHAR *check_connection_query;

    static bool CheckResult(SQLRETURN rc, const std::string& log_message, SQLHANDLE handle, int32_t handle_type);
    static bool CheckConnection(SQLHDBC conn);
    static void Cleanup(SQLHENV henv, SQLHDBC conn, SQLHSTMT hstmt);
    static bool AllocateHandle(SQLSMALLINT handle_type, SQLHANDLE input_handle, SQLHANDLE output_handle, const std::string& log_message);
    static bool ExecuteQuery(SQLHSTMT stmt, SQLTCHAR* query, const std::string& log_message);
    static bool BindColumn(SQLHSTMT stmt, SQLUSMALLINT col_num, SQLSMALLINT type, SQLPOINTER dest, SQLLEN dest_size, const std::string& log_message);
    static bool FetchResults(SQLHSTMT stmt, const std::string& log_message);

private:
    static void LogMessage(const std::string& log_message, SQLHANDLE handle, int32_t handle_type);     
};

class IOdbcHelper {
public:
    virtual bool CheckResult(SQLRETURN rc, const std::string& log_message, SQLHANDLE handle, int32_t handle_type) = 0;
    virtual bool CheckConnection(SQLHDBC conn) = 0;
    virtual void Cleanup(SQLHENV henv, SQLHDBC conn, SQLHSTMT hstmt) = 0;
    virtual bool AllocateHandle(SQLSMALLINT handle_type, SQLHANDLE input_handle, SQLHANDLE output_handle, const std::string& log_message) = 0;
    virtual bool ExecuteQuery(SQLHSTMT stmt, SQLTCHAR* query, const std::string& log_message) = 0;
    virtual bool BindColumn(SQLHSTMT stmt, SQLUSMALLINT col_num, SQLSMALLINT type, SQLPOINTER dest, SQLLEN dest_size, const std::string& log_message) = 0;
    virtual bool FetchResults(SQLHSTMT stmt, const std::string& log_message) = 0;
};

class OdbcHelperWrapper : public IOdbcHelper {
public:
    bool CheckResult(SQLRETURN rc, const std::string& log_message, SQLHANDLE handle, int32_t handle_type) override { return OdbcHelper::CheckResult(rc, log_message, handle, handle_type); };
    bool CheckConnection(SQLHDBC conn) override { return OdbcHelper::CheckConnection(conn); };
    void Cleanup(SQLHENV henv, SQLHDBC conn, SQLHSTMT hstmt) override { OdbcHelper::Cleanup(henv, conn, hstmt); };
    bool AllocateHandle(SQLSMALLINT handle_type, SQLHANDLE input_handle, SQLHANDLE output_handle, const std::string& log_message) override { return OdbcHelper::AllocateHandle(handle_type, input_handle, output_handle, log_message); };
    bool ExecuteQuery(SQLHSTMT stmt, SQLTCHAR* query, const std::string& log_message) override { return OdbcHelper::ExecuteQuery(stmt, query, log_message); };
    bool BindColumn(SQLHSTMT stmt, SQLUSMALLINT col_num, SQLSMALLINT type, SQLPOINTER dest, SQLLEN dest_size, const std::string& log_message) override { return OdbcHelper::BindColumn(stmt, col_num, type, dest, dest_size, log_message); };
    bool FetchResults(SQLHSTMT stmt, const std::string& log_message) override { return OdbcHelper::FetchResults(stmt, log_message); };
};

#endif // ODBCHELPER_H_
