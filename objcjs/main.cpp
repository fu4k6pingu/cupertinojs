//
//  main.cpp
//  objcjs
//
//  Created by Jerry Marino on 7/5/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//


#include "objccodegen.h"
#include <assert.h>
//TODO : is there a better way to handle these v8 imports?
#include "src/compiler.h"
#include "src/api.h"
#include "src/v8.h"
#include <src/parser.h>
#include <tools/shell-utils.h>
#include <src/prettyprinter.h>
#include <include/libplatform/libplatform.h>
#include <vector>

#define _DEBUG 1

using namespace v8::internal;

CompilationInfoWithZone *ProgramWithSourceHandle(v8::Handle<v8::String> source_handle){
    v8::base::TimeDelta parse_time1, parse_time2;
    Handle<String> handle = v8::Utils::OpenHandle(*source_handle);
    Handle<Script> script = Isolate::Current()->factory()->NewScript(handle);
    
    i::ScriptData* cached_data_impl = NULL;
    // First round of parsing (produce data to cache).
    auto info = new CompilationInfoWithZone(script);
    info->MarkAsGlobal();
    info->MarkNonOptimizable();
    info->MarkAsEval();
    assert(!info->is_lazy());
    
    info->SetCachedData(&cached_data_impl, i::PRODUCE_CACHED_DATA);
    
    bool success = Parser::Parse(info, false);
    if (!success) {
        fprintf(stderr, "Parsing failed\n");
        abort();
    }
#if _DEBUG
    PrintF(AstPrinter(info->zone()).PrintProgram(info->function()));
#endif
    return info;
}
class StringResource8 : public v8::String::ExternalAsciiStringResource {
public:
    StringResource8(const char* data, int length)
    : data_(data), length_(length) { }
    virtual size_t length() const { return length_; }
    virtual const char* data() const { return data_; }
    
private:
    const char* data_;
    int length_;
};

v8::Handle<v8::String>SourceHandleWithName(char *sourceName, v8::Isolate *isolate){
    int length = 0;
    const byte* source = ReadFileAndRepeat(sourceName, &length, 1);

    v8::Handle<v8::String> source_handle;
    Encoding encoding = LATIN1;
    
    switch (encoding) {
        case UTF8: {
            source_handle = v8::String::NewFromUtf8(
                                                    isolate, reinterpret_cast<const char*>(source));
            break;
        }
        case UTF16: {
            source_handle = v8::String::NewFromTwoByte(
                                                       isolate, reinterpret_cast<const uint16_t*>(source),
                                                       v8::String::kNormalString, length / 2);
            break;
        }
        case LATIN1: {
            StringResource8* string_resource;
            string_resource =
            new StringResource8(reinterpret_cast<const char*>(source), length);
            source_handle = v8::String::NewExternal(isolate, string_resource);
            
            break;
        }
    }
    return source_handle;
}

//TODO (unused): support multiple names
std::vector<std::string> parseNames(int argc, const char * argv[]){
    std::vector<std::string> fnames;
    std::string benchmark;
    int repeat = 1;
    Encoding encoding = LATIN1;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--latin1") == 0) {
            encoding = LATIN1;
        } else if (strcmp(argv[i], "--utf8") == 0) {
            encoding = UTF8;
        } else if (strcmp(argv[i], "--utf16") == 0) {
            encoding = UTF16;
        } else if (strncmp(argv[i], "--benchmark=", 12) == 0) {
            benchmark = std::string(argv[i]).substr(12);
        } else if (strncmp(argv[i], "--repeat=", 9) == 0) {
            std::string repeat_str = std::string(argv[i]).substr(9);
            repeat = atoi(repeat_str.c_str());
        } else if (i > 0 && argv[i][0] != '-') {
            fnames.push_back(std::string(argv[i]));
        }
    }
    return fnames;
}

int main(int argc, const char * argv[])
{
    //Init a v8 stack
    v8::V8::InitializeICU();
    v8::Platform* platform = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform);
   
    //Init an isolate
    //TODO : should this go somehwere else?
    v8::Isolate* isolate = v8::Isolate::New();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global);
    ASSERT(!context.IsEmpty());
    v8::Context::Scope scope(context);

    auto module = ProgramWithSourceHandle(SourceHandleWithName("/Users/jerrymarino/Projects/objcjs/test-js/test.js", isolate));
    auto codegen = ObjCCodeGen(module->zone());
    codegen.Visit(module->function());
    codegen.dump();
    
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete platform;
    return 0;
}
