
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

#include "html_util.h"
#include "../util/logger_wrapper.h"

const std::unordered_map<std::string, char> HtmlUtil::HTML_DECODE_MAP = {
    {"&#x2b;", '+'},
    {"&#x3d;", '='},
    {"&lt;", '<'},
    {"&gt;", '>'},
    {"&amp;", '&'},
    {"&apos;", '\''},
    {"&quot;", '"'}
};

std::string HtmlUtil::escape_html_entity(const std::string& html) {
    std::string retval;
    DLOG(INFO) << "Before HTML escape modification: " << html;
    int i = 0;
    int length = html.length();
    while (i < length) {
        char c = html[i];
        if (c != '&') {
            retval.append(1, c);
            i++;
            continue;
        }

        int semicolon_idx = html.find(';', i);
        if (semicolon_idx != std::string::npos) {
            std::string html_code = html.substr(i, semicolon_idx - i + 1);
            if (auto itr = HtmlUtil::HTML_DECODE_MAP.find(html_code); itr != HtmlUtil::HTML_DECODE_MAP.end()) {
                retval.append(1, itr->second);
                i = semicolon_idx + 1;
            }
        } else {
            retval.append(1, c);
            i++;
        }

    }
    DLOG(INFO) << "After HTML escape modification: " << html;
    return retval;
}
