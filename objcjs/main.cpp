//
//  main.cpp
//  objcjs
//
//  Created by Jerry Marino on 7/5/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include <include/libplatform/libplatform.h>
#include "compiler.h"

using namespace objcjs;
using namespace v8::internal;

int main(int argc, const char * argv[])
{
    //Init a v8 stack
    v8::V8::InitializeICU();
    v8::Platform* platform = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform);
   
    //Init an isolate
    v8::Isolate* isolate = v8::Isolate::New();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global);
    ASSERT(!context.IsEmpty());
    v8::Context::Scope scope(context);

    objcjs::CompilerOptions options(argc, argv);
    if (!options.validate()) {
        return 1;
    }
  
    objcjs::Compiler(options, isolate).run();
    
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete platform;
    return 0;
}

