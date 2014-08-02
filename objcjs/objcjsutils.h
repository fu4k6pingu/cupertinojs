//
//  objcjsutils.h
//  objcjs
//
//  Created by Jerry Marino on 8/4/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#ifndef __objcjs__objcjsutils__
#define __objcjs__objcjsutils__

#include <iostream>
#include <stdarg.h>  // for va_start, etc
#include <memory>    // for std::unique_ptr
#include <sstream>
#include <vector>

namespace objcjs {
    std::string string_format(const std::string fmt_str, ...);
    std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
    std::vector<std::string> split(const std::string &s, char delim);
    std::string get_env_var(std::string const & key );
}


#endif /* defined(__objcjs__objcjsutils__) */
