
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
