//
//  objcjscompiler.h
//  objcjs
//
//  Created by Jerry Marino on 7/26/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#ifndef objcjs_compiler_h
#define objcjs_compiler_h

#include <string>
#include <vector>
#include <iostream>

namespace objcjs {
    class CompilerOptions{
    public:
        std::string _runtimePath;
        std::string _buildDir;
        std::string _mTripel;
        bool _createExecutable;
        
        std::vector<std::string> _names;
        bool _debug;
       
        CompilerOptions(int argc, const char * argv[]);
        int validate();
    };
    
    class Compiler {
        CompilerOptions *_options;
        v8::Isolate *_isolate;
        std::string compileModule(v8::Isolate *isolate, std::string filePath);
        
    public:
        Compiler(CompilerOptions options, v8::Isolate *isolate);
        void run();
    };
}

#endif
