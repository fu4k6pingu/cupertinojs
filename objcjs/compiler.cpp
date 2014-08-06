//
//  compiler.cpp
//  objcjs
//
//  Created by Jerry Marino on 7/26/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include <assert.h>
#include <vector>
#include <iostream>
#include <fstream>

#include <src/parser.h>
#include <src/prettyprinter.h>
#include <tools/shell-utils.h>
#include <include/libplatform/libplatform.h>

#include "llvm/IR/Module.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Bitcode/ReaderWriter.h"

#include "objcjsutils.h"
#include "cgobjcjs.h"

#include "compiler.h"

using namespace v8::internal;
using namespace objcjs;


//TODO : determine distro for llvm
const char *LLVM_LLC_PATH = "/usr/local/Cellar/llvm/3.4/bin/llc";

const char *COMPILE_ENV_BUILD_DIR = "OBJCJS_ENV_BUILD_DIR";
const char *COMPILE_ENV_OBJCJS_RUNTIME_PATH = "OBJCJS_ENV_RUNTIME";
const char *COMPILE_ENV_DEBUG = "OBJCJS_ENV_DEBUG_COMPILER";

#pragma mark - CompilerOptions


std::vector<std::string> ParseNames(int argc, const char * argv[]){
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

objcjs::CompilerOptions::CompilerOptions(int argc, const char * argv[]){
    _names = ParseNames(argc, argv);
    _runtimePath = get_env_var(COMPILE_ENV_OBJCJS_RUNTIME_PATH);
    _buildDir = get_env_var(COMPILE_ENV_BUILD_DIR);
    _debug = get_env_var(COMPILE_ENV_DEBUG) == "true";
}

int objcjs::CompilerOptions::validate(){
    if(!_names.size()){
        std::cout << "no input files \n";
        return 0;
    }
    if(!_runtimePath.length()){
        std::cout << "missing runtime path \n";
        return 0;
    }
    
    if(!_buildDir.length()){
        std::cout << "missing build dir \n";
        return 0;
    }
    
    return 1;
}

#pragma mark - Compiler

objcjs::Compiler::Compiler(CompilerOptions options, v8::Isolate *isolate){
    _options = &options;
    _isolate = isolate;
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

v8::Handle<v8::String>SourceHandleWithName(const char *sourceName, v8::Isolate *isolate){
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

CompilationInfoWithZone *ProgramWithSourceHandle(v8::Handle<v8::String> source_handle, objcjs::CompilerOptions options){
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
    
    if (options._debug) {
        PrintF(AstPrinter(info->zone()).PrintProgram(info->function()));
    }
    
    return info;
}

void runModule(llvm::Module module){
//    llvm::ExecutionEngine *engine = llvm::EngineBuilder(module).create();
//    engine->runFunction(main_func, std::vector<llvm::GenericValue>());
}

std::string objcjs::Compiler::compileModule(v8::Isolate *isolate, std::string filePath){
    std::string buildDir = get_env_var(COMPILE_ENV_BUILD_DIR);
    std::string fileName = split(filePath, '/').back();
    std::string moduleName = split(fileName, '.').front();
    
    assert(moduleName.length() && "missing file");
    
    auto module = ProgramWithSourceHandle(SourceHandleWithName(filePath.c_str(), isolate), *_options);
    
    CGObjCJS codegen = CGObjCJS(module->zone(), moduleName, module);
    codegen.Visit(module->function());
    if (_options->_debug) {
        codegen.dump();
    }
    
    llvm::verifyModule(*codegen._module, llvm::PrintMessageAction);
    llvm::PassManager PM;
    std::string error;
    std::string out;
    
    llvm::raw_string_ostream file(out);
    PM.add(createPrintModulePass(&file));
    PM.run(*codegen._module);
    
    std::string outFileName = string_format("%s/%s.bc", buildDir.c_str(), moduleName.c_str());
    
    std::ofstream outfile;
    outfile.open(outFileName);
    outfile << out;
    outfile.close();
    
    //compile bitcode
    system(string_format("%s %s",
                         LLVM_LLC_PATH,
                         outFileName.c_str()).c_str());
    std::string llcOutput = string_format("%s/%s.s", buildDir.c_str(), moduleName.c_str());
    return llcOutput;
}

void objcjs::Compiler::run(){
    std::string sFiles;
    CompilerOptions options = *_options;
    for (unsigned i = 0; i < options._names.size(); i++){
        std::string filePath = options._names[i];
        std::string sFile = compileModule(_isolate, filePath);
        sFiles += " ";
        sFiles += sFile;
    }
    
    std::string clangCmd = string_format("clang -framework Foundation %s %s -o %s/objcjsapp",
                                         options._runtimePath.c_str(),
                                         sFiles.c_str(),
                                         options._buildDir.c_str());
    system(clangCmd.c_str());
}
