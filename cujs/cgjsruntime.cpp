//
//  cgjsruntime.cpp
//  cujs
//
//  Created by Jerry Marino on 7/19/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include <malloc/malloc.h>
#include <string>

#include "cgjsruntime.h"

using namespace cujs;

std::string cujs::ObjCSelectorToJS(std::string objCSelector){
    std::string jsSelector;
    size_t length = objCSelector.length();
    int remove = 0;
    int jsSelectorPos = 0;
    for (int i = 0; i < length; i++) {
        char current = objCSelector.at(i);
        if (current == ':'){
            i++;
            remove++;
            if (i == length) {
                break;
            }
            current = objCSelector.at(i);
            jsSelector += toupper(current);
        } else {
            jsSelector += current;
        }
        jsSelectorPos++;
    }

    return jsSelector;
}

const auto _ObjcPointerTy = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);

llvm::Type *cujs::ObjcPointerTy(){
    return _ObjcPointerTy;
}

llvm::Value *cujs::ObjcNullPointer() {
    auto ty = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);
    return llvm::ConstantPointerNull::get(ty);
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
llvm::AllocaInst *cujs::CreateEntryBlockAlloca(llvm::Function *Function,
                                          const std::string &VarName) {
    llvm::IRBuilder<> Builder(&Function->getEntryBlock(),
                     Function->getEntryBlock().begin());
    return Builder.CreateAlloca(ObjcPointerTy(), 0, VarName.c_str());
}

llvm::Function *ObjcCodeGenFunction(size_t numParams,
                                    std::string name,
                                    llvm::Module *mod) {
    std::vector<llvm::Type*> Params(numParams, ObjcPointerTy());
    llvm::FunctionType *FT = llvm::FunctionType::get(ObjcPointerTy(), Params, false);
    return llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, mod);
}

llvm::Function *ObjcCodeGenFunction(size_t numParams,
                                    std::string name,
                                    llvm::Module *mod,
                                    bool isVarArg) {
    std::vector<llvm::Type*> Params(numParams, ObjcPointerTy());
    llvm::FunctionType *FT = llvm::FunctionType::get(ObjcPointerTy(), Params, true);
    return llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, mod);
}


llvm::Function *cujs::CGJSFunction(size_t numParams,
                                 std::string name,
                                 llvm::Module *mod) {
    std::vector<llvm::Type*> Params;

    Params.push_back(ObjcPointerTy());
    Params.push_back(ObjcPointerTy());
    
    for (int i = 0; i < numParams; i++) {
        Params.push_back(ObjcPointerTy());
    }
    
    llvm::FunctionType *FT = llvm::FunctionType::get(ObjcPointerTy(), Params, true);
    return llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, mod);
}

llvm::Function *cujs::ObjcCodeGenModuleInit(llvm::IRBuilder<>*builder,
                                      llvm::Module *module,
                                      std::string name) {
    std::vector<llvm::Type*> argumentTypes;
    argumentTypes.push_back(ObjcPointerTy());
    llvm::FunctionType *functionType = llvm::FunctionType::get(
                                                        ObjcPointerTy(),
                                                        argumentTypes,
                                                        false);
    
    llvm::Function *function = llvm::Function::Create(
                                                  functionType,
                                                  llvm::Function::ExternalLinkage,
                                                  llvm::Twine(name),
                                                  module);
    
    function->setCallingConv(llvm::CallingConv::C);
    
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(module->getContext(), "entry", function);
    builder->SetInsertPoint(BB);
   
    builder->saveAndClearIP();
    return function;
}


void cujs::SetModuleCtor(llvm::Module *module, llvm::Function *cTor){
    llvm::Function* func_init = module->getFunction("cujs_module_init");
    assert(!func_init && "module already has a ctor");
    
    std::vector<llvm::Type*>FuncTy_args;
    llvm::FunctionType* FuncTy = llvm::FunctionType::get(
                                                         llvm::Type::getVoidTy(module->getContext()),
                                                         FuncTy_args,
                                                         false);
    func_init = llvm::Function::Create(
                                       FuncTy,
                                       llvm::GlobalValue::ExternalLinkage,
                                       "cujs_module_init", module);
    func_init->setCallingConv(llvm::CallingConv::C);
    
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(module->getContext(), "entry", func_init);
    
    std::vector<llvm::Value *>Args;
    Args.push_back(ObjcNullPointer());
    auto callModuleFunction = llvm::CallInst::Create(cTor,Args);

    BB->getInstList().push_front(callModuleFunction);
    llvm::ReturnInst::Create(module->getContext(), BB);
    
    std::vector<llvm::Type*>StructTy_fields;
    StructTy_fields.push_back(llvm::IntegerType::get(module->getContext(), 32));
    StructTy_fields.push_back(func_init->getType());
    
    llvm::StructType *StructTy = llvm::StructType::get(module->getContext(), StructTy_fields, false);
    std::vector<llvm::Constant*> const_array_elems;

    std::vector<llvm::Constant*> const_struct_fields;
    llvm::ConstantInt* const_int32_99 = llvm::ConstantInt::get(module->getContext(),llvm:: APInt(32, llvm::StringRef("65535"), 10));
    const_struct_fields.push_back(const_int32_99);
    const_struct_fields.push_back(func_init);
    llvm::Constant* const_struct = llvm::ConstantStruct::get(StructTy, const_struct_fields);
    
    const_array_elems.push_back(const_struct);
    
    llvm::ArrayType* ArrayTy = llvm::ArrayType::get(StructTy, 1);
    llvm::Constant* const_array = llvm::ConstantArray::get(ArrayTy, const_array_elems);
    llvm::GlobalVariable* gvar_array_llvm_global_ctors = new llvm::GlobalVariable(*module,
                                                                                  ArrayTy,
                                                                                  false,
                                                                                  llvm::GlobalValue::AppendingLinkage,
                                                                                  0,
                                                                                  "llvm.global_ctors");
    gvar_array_llvm_global_ctors->setInitializer(const_array);
}

#define DefExternFucntion(name){\
{\
    std::vector<llvm::Type*> ArgumentTypes; \
    ArgumentTypes.push_back(ObjcPointerTy()); \
    llvm::FunctionType *ft = llvm::FunctionType::get(ObjcPointerTy(), ArgumentTypes, true); \
    auto function = llvm::Function::Create( \
        ft, llvm::Function::ExternalLinkage, \
        llvm::Twine(name), \
        _module );\
    function->setCallingConv(llvm::CallingConv::C); \
} \
}

static llvm::Function *ObjcMsgSendFPret(llvm::Module *module) {
        std::vector<llvm::Type*> ArgumentTypes;
    ArgumentTypes.push_back(ObjcPointerTy());
    auto doubleTy  = llvm::Type::getDoubleTy(llvm::getGlobalContext());
    llvm::FunctionType *ft = llvm::FunctionType::get(doubleTy, ArgumentTypes, true); \
    auto function = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage,
                                           llvm::Twine("objc_msgSend_fpret"), \
        module );\
    function->setCallingConv(llvm::CallingConv::C);
    return function;
}

static llvm::Function* ObjcNSLogPrototye(llvm::Module *module)
{
    std::vector<llvm::Type*> ArgumentTypes;
    ArgumentTypes.push_back(ObjcPointerTy());
    
    llvm::FunctionType* functionType =
    llvm::FunctionType::get(
                            llvm::Type::getInt32Ty(module->getContext()), ArgumentTypes, true);
    
    llvm::Function *function = llvm::Function::Create(
                                                  functionType, llvm::Function::ExternalLinkage,
                                                  llvm::Twine("NSLog"),
                                                  module
                                                  );
    function->setCallingConv(llvm::CallingConv::C);
    return function;
}

static llvm::Function *ObjcMallocPrototype(llvm::Module *module) {
    std::vector<llvm::Type*>ArgumentTypes;
    ArgumentTypes.push_back(llvm::IntegerType::get(module->getContext(), 64));
    llvm::FunctionType* functionType = llvm::FunctionType::get(
                                                               ObjcPointerTy(),
                                                               ArgumentTypes,
                                                               false);
    llvm::Function* function = module->getFunction("malloc");
    if (!function) {
        function = llvm::Function::Create(
                                          functionType,
                                          llvm::GlobalValue::ExternalLinkage,
                                          "malloc", module); // (external, no body)
        function->setCallingConv(llvm::CallingConv::C);
    }
    
    return function;
}


llvm::Value *cujs::NewLocalStringVar(const char* data,
                            size_t len,
                            llvm::Module *module) {
    llvm::Constant *constTy = llvm::ConstantDataArray::getString(llvm::getGlobalContext(), data);
    auto type = llvm::ArrayType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), len + 1);
    llvm::GlobalVariable *var = new llvm::GlobalVariable(*module,
                                                         type,
                                                         true,
                                                         llvm::GlobalValue::PrivateLinkage,
                                                         0,
                                                         ".str");
    var->setInitializer(constTy);
    
    std::vector<llvm::Constant*> const_ptr_16_indices;
    llvm::ConstantInt* const_int32_17 =llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, llvm::StringRef("0"), 10));
    const_ptr_16_indices.push_back(const_int32_17);
    const_ptr_16_indices.push_back(const_int32_17);
    llvm::Constant* const_ptr_16 = llvm::ConstantExpr::getGetElementPtr(var, const_ptr_16_indices);
    auto castedConst = llvm::ConstantExpr::getPointerCast(const_ptr_16, ObjcPointerTy());
    return castedConst;
}

llvm::Value *cujs::NewLocalStringVar(std::string value,
                            llvm::Module *module) {
    return cujs::NewLocalStringVar(value.c_str(), value.size(), module);
}

#pragma mark - Runtime calls

CGJSRuntime::CGJSRuntime(llvm::IRBuilder<> *builder,
                                 llvm::Module *module){
    _builder = builder;
    _module = module;
   
    _builtins.insert("NSLog");
    
    DefExternFucntion("sel_getUid");
    DefExternFucntion("objc_getClass");

    //JSRuntime
    DefExternFucntion("cujs_invoke");
    DefExternFucntion("cujs_defineJSFunction");
   
    ObjcCodeGenFunction(0, "cujs_newJSObjectClass", _module);
    ObjcCodeGenFunction(3, "cujs_assignProperty", _module);

    //Objc runtim
    ObjcCodeGenFunction(0, "objc_msgSend", _module, true);
    
    ObjcMsgSendFPret(_module);
    ObjcMallocPrototype(_module);
    ObjcNSLogPrototye(_module);
}

llvm::Value *CGJSRuntime::newString(std::string string){
    const char *name = string.c_str();
    llvm::Value *strGlobal = NewLocalStringVar(name, string.length(), _module);
    return messageSend(classNamed("NSString"), "stringWithUTF8String:", strGlobal);
}

llvm::Value *CGJSRuntime::classNamed(const char *name){
    return _builder->CreateCall(_module->getFunction("objc_getClass"),  NewLocalStringVar(std::string(name), _module), "calltmp-getclass");
}

llvm::Value *CGJSRuntime::newNumber(double doubleValue){
    auto constValue = llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(doubleValue));
    return messageSend(classNamed("NSNumber"), "numberWithDouble:", constValue);
}

llvm::Value *CGJSRuntime::newObject() {
    auto newClass = _builder->CreateCall(_module->getFunction("cujs_newJSObjectClass"));
    return messageSend(newClass, "cujs_new");
}

llvm::Value *CGJSRuntime::newObject(std::vector<llvm::Value *>values) {
    UNIMPLEMENTED();
    auto newClass = _builder->CreateCall(_module->getFunction("cujs_newJSObjectClass"));
    return messageSend(newClass, "cujs_withObjectsAndKeys:", values);
}

llvm::Value *CGJSRuntime::newArray(std::vector<llvm::Value *>values) {
    return messageSend(classNamed("CUJSMutableArray"), "arrayWithObjects:", values);
}

llvm::Value *CGJSRuntime::newNumberWithLLVMValue(llvm::Value *doubleValue){
    return messageSend(classNamed("NSNumber"), "numberWithDouble:", doubleValue);
}

llvm::Value *CGJSRuntime::doubleValue(llvm::Value *llvmValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), NewLocalStringVar(std::string("doubleValue"), _module), "calltmp");

    std::vector<llvm::Value*> Args;
    Args.push_back(llvmValue);
    Args.push_back(sel);

    return _builder->CreateCall(_module->getFunction("objc_msgSend_fpret"), Args, "calltmp-objc_msgSend_fpret");
}

llvm::Value *CGJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *selectorName,
                                          llvm::Value *Arg) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), NewLocalStringVar(std::string(selectorName), _module), "calltmp");
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selector);
   
    if (Arg){
        Args.push_back(Arg);
    }
   
    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "calltmp-objc_msgSend");
}

llvm::Value *CGJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *selectorName,
                                          std::vector<llvm::Value *>ArgsV) {
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selectorWithName(selectorName));
    for (int i = 0; i < ArgsV.size(); i++) {
        Args.push_back(ArgsV.at(i));
    }
  
    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "calltmp-objc_msgSend");

}

llvm::Value *CGJSRuntime::messageSendFP(llvm::Value *receiver,
                                          const char *selectorName,
                                          std::vector<llvm::Value *>ArgsV) {
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selectorWithName(selectorName));
    for (int i = 0; i < ArgsV.size(); i++) {
        Args.push_back(ArgsV.at(i));
    }
  
    return _builder->CreateCall(_module->getFunction("objc_msgSend_fpret"), Args, "calltmp-objc_msgSend");
}

llvm::Value *CGJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *name) {
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selectorWithName(name));
    llvm::Function *function = _module->getFunction("objc_msgSend");
    return _builder->CreateCall(function, Args, "calltmp-objc_msgSend");
}

llvm::Value *CGJSRuntime::selectorWithName(const char *name){
    return _builder->CreateCall(_module->getFunction("sel_getUid"), NewLocalStringVar(std::string(name), _module), "calltmp");
}

llvm::Value *CGJSRuntime::messageSendProperty(llvm::Value *receiver,
                                 const char *name,
                                 std::vector<llvm::Value *>ArgsV) {
    return messageSend(receiver, name, ArgsV);
}

//Sends a message to a JSFunction instance
llvm::Value *CGJSRuntime::invokeJSValue(llvm::Value *instance,
                                                    std::vector<llvm::Value *>ArgsV) {
    std::vector<llvm::Value*> Args;
    Args.push_back(instance);

    for (int i = 0; i < ArgsV.size(); i++) {
        Args.push_back(ArgsV.at(i));
    }

    Args.push_back(ObjcNullPointer());
    return _builder->CreateCall(_module->getFunction("cujs_invoke"), Args, "cujs_invoke");
}

//Convert a llvm value to an bool
//which is represented as an int in JS land
llvm::Value *CGJSRuntime::boolValue(llvm::Value *llvmValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), NewLocalStringVar(std::string("cujs_boolValue"), _module), "calltmp");

    std::vector<llvm::Value*> Args;
    Args.push_back(llvmValue);
    Args.push_back(sel);

    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "objc_msgSend");
}

llvm::Value *CGJSRuntime::defineJSFuction(const char *name,
                                              unsigned nArgs
                                                 ) {
    auto function = CGJSFunction(nArgs, name, _module);
    
    auto nameAlloca = NewLocalStringVar(name, strlen(name), _module);
    
    std::vector<llvm::Value*> Args;
    Args.push_back(nameAlloca);
    Args.push_back(function);

    return llvm::CallInst::Create(_module->getFunction("cujs_defineJSFunction"), Args, "calltmp");
}

llvm::Value *CGJSRuntime::declareProperty(llvm::Value *instance,
                                              std::string name) {
    
    auto nameAlloca = NewLocalStringVar(name.c_str(), name.length(), _module);
    return messageSend(instance, "cujs_defineProperty:", nameAlloca);
}

llvm::Value *CGJSRuntime::assignProperty(llvm::Value *instance,
                                             std::string name,
                                             llvm::Value *value) {
    auto nameAlloca = NewLocalStringVar(name.c_str(), name.length(), _module);
    std::vector<llvm::Value*> Args;
    Args.push_back(instance);
    Args.push_back(nameAlloca);
    Args.push_back(value);
    
    return _builder->CreateCall(_module->getFunction("cujs_assignProperty"), Args, "calltmp");
}

llvm::Value *CGJSRuntime::declareGlobal(std::string name) {
    llvm::GlobalVariable* var = new llvm::GlobalVariable(*_module,
                                                         ObjcPointerTy(),
                                                         false,
                                                         llvm::GlobalValue::ExternalLinkage,
                                                         0,
                                                         name);
    auto ty = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);
    var->setInitializer(llvm::ConstantPointerNull::get(ty));
    return var;
}

bool CGJSRuntime::isBuiltin(std::string name) {
    return _builtins.count(name) > 0;
}