//
//  utils.h
//  objcjs
//
//  Created by Jerry Marino on 7/26/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#ifndef objcjs_utils_h
#define objcjs_utils_h

#include <stdarg.h>  // for va_start, etc
#include <memory>    // for std::unique_ptr
#include <sstream>

namespace objcjs {
    std::string string_format(const std::string fmt_str, ...) {
        int final_n, n = ((int)fmt_str.size()) * 2; /* reserve 2 times as much as the length of the fmt_str */
        std::string str;
        std::unique_ptr<char[]> formatted;
        va_list ap;
        while(1) {
            formatted.reset(new char[n]); /* wrap the plain char array into the unique_ptr */
            strcpy(&formatted[0], fmt_str.c_str());
            va_start(ap, fmt_str);
            final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
            va_end(ap);
            if (final_n < 0 || final_n >= n)
                n += abs(final_n - n + 1);
            else
                break;
        }
        return std::string(formatted.get());
    }
    
    std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }
    
    static std::vector<std::string> split(const std::string &s, char delim) {
        std::vector<std::string> elems;
        split(s, delim, elems);
        return elems;
    }
}

#endif
