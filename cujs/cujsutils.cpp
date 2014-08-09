//
//  cujsutils.cpp
//  cujs
//
//  Created by Jerry Marino on 8/4/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include "cujsutils.h"

using namespace cujs;

std::string cujs::string_format(const std::string fmt_str, ...) {
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

std::vector<std::string> &cujs::split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> cujs::split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    cujs::split(s, delim, elems);
    return elems;
}

std::string cujs::get_env_var(std::string const & key ) {
    char * val;
    val = getenv( key.c_str() );
    std::string retval = "";
    if (val != NULL) {
        retval = val;
    }
    return retval;
}

