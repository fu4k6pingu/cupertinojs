//
//  cujsruntime.h
//  cujs
//
//  Created by Jerry Marino on 7/19/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#ifndef cujs_cujsruntime_h
#define cujs_cujsruntime_h

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm-c/Core.h>
#include <src/ast.h>
#include <set>
#include "cgclang.h"

namespace cujs {
    struct ObjCMethod;
    struct ObjCStruct;
    struct ObjCClass;
    struct ObjCTypeDef;

    std::string ObjCSelectorToJS(std::string objCSelector);
    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *Function,
                                             const std::string &VarName);
    llvm::Value *NewLocalStringVar(std::string value, llvm::Module *module);
    llvm::Function *CGJSFunction(size_t numParams,
                                     std::string name,
                                     llvm::Module *mod);
    
    llvm::Function *ObjcCodeGenModuleInit(llvm::IRBuilder<>*builder,
                                          llvm::Module *module,
                                          std::string name);
    llvm::Type *ObjcPointerTy();
    llvm::Value *ObjcNullPointer();
    llvm::Value *NewLocalStringVar(const char* data, size_t len, llvm::Module *module);
    
    void SetModuleCtor(llvm::Module *mod, llvm::Function *cTor);
    
    class CGJSRuntime {
        std::set <std::string>  _builtins;
    public:
        
        std::map <std::string, ObjCMethod *> _objCMethodBySelector;
        std::set <std::string> _classes;
        std::set <std::string> _structs;
        std::map <std::string, ObjCTypeDef *> _typedefs;
        std::map <std::string, ObjCStruct *> _objCStructByName;
        
        CGJSRuntime(llvm::IRBuilder<> *builder, llvm::Module *module);
        
        llvm::IRBuilder<> *_builder;
        llvm::Module *_module;
        
        llvm::Value *newString(std::string string);
        llvm::Value *newNumber(double value);
        llvm::Value *newObject();
        llvm::Value *newObject(std::vector<llvm::Value *>values);
        llvm::Value *newArray(std::vector<llvm::Value *>values);
        
        llvm::Value *newNumberWithLLVMValue(llvm::Value *value);
        
        llvm::Value *doubleValue(llvm::Value *llvmValue);
        llvm::Value *boolValue(llvm::Value *llvmValue);
        llvm::Value *messageSend(llvm::Value *receiver,
                                 const char *selector,
                                 std::vector<llvm::Value *>ArgsV);
        llvm::Value *messageSendFP(llvm::Value *receiver,
                                   const char *selectorName,
                                   std::vector<llvm::Value *>ArgsV);
        llvm::Value *messageSend(llvm::Value *receiver,
                                 const char *selector,
                                 llvm::Value *Arg);
        llvm::Value *messageSend(llvm::Value *receiver,
                                 const char *selector);
        
        llvm::Value *messageSendProperty(llvm::Value *receiver,
                                         const char *selector,
                                         std::vector<llvm::Value *>ArgsV);
        llvm::Value *invokeJSValue(llvm::Value *instance,
                                   std::vector<llvm::Value *>ArgsV);
        llvm::Value *classNamed(const char *name);
        llvm::Value *defineJSFuction(const char *name,
                                     unsigned nArgs
                                     );
       
        
        llvm::Value *declareProperty(llvm::Value *instance,
                                     std::string name);
        llvm::Value *assignProperty(llvm::Value *instance,
                                    std::string name,
                                    llvm::Value *value);
        
        llvm::Value *declareGlobal(std::string name);
       
        void enterStruct(ObjCStruct *newStruct);
        void enterClass(ObjCClass *newClass);
        void enterTypeDef(ObjCTypeDef *typeDef);
        
        llvm::Value *selectorByAddingColon(const char *name);
        llvm::Value *selectorWithName(const char *name);
        
        bool isBuiltin(std::string name);
    };
};

#endif
