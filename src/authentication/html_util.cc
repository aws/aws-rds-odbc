
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

#ifndef XCODE_BUILD
#include "../util/logger_wrapper.h"
#endif

namespace HtmlUtil {
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

            if (html.substr(i, 6) == "&#x2b;") {
                retval.append(1,'+');
                i += 6;
            } else if (html.substr(i, 6) == "&#x3d;") {
                retval.append(1, '=');
                i += 6;
            } else if (html.substr(i, 4) == "&lt;") {
                retval.append(1,'<');
                i += 4;
            } else if (html.substr(i, 4) == "&gt;") {
                retval.append(1, '>');
                i += 4;
            } else if (html.substr(i, 5) == "&amp;") {
                retval.append(1, '&');
                i += 5;
            } else if (html.substr(i, 6) == "&apos;") {
                retval.append(1, '\'');
                i += 6;
            } else if (html.substr(i, 6) == "&quot;") {
                retval.append(1, '"');
                i += 6;
            } else {
                retval.append(1, c);
                ++i;
            }
        }
        DLOG(INFO) << "After HTML escape modification: " << html;
        return retval;
    }
};
