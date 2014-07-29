//
//  cgobjcjsruntime.cpp
//  objcjs
//
//  Created by Jerry Marino on 7/19/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include <malloc/malloc.h>
#include "cgobjcjsruntime.h"
#include "string.h"

const auto _ObjcPointerTy = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);

llvm::Type *ObjcPointerTy(){
    return _ObjcPointerTy;
}

llvm::Value *ObjcNullPointer() {
    auto ty = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);
    return llvm::ConstantPointerNull::get(ty);
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *Function,
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


llvm::Function *CGObjCJSFunction(size_t numParams,
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

llvm::Function *ObjcCodeGenMainPrototype(llvm::IRBuilder<>*builder,
                                         llvm::Module *module) {
    std::vector<llvm::Type*> argumentTypes;
    
    llvm::FunctionType *functionType = llvm::FunctionType::get(
                                                        llvm::Type::getInt32Ty(module->getContext()),
                                                        argumentTypes,
                                                        false);
    
    llvm::Function *function = llvm::Function::Create(
                                                  functionType,
                                                  llvm::Function::ExternalLinkage,
                                                  llvm::Twine("main"),
                                                  module);
    
    function->setCallingConv(llvm::CallingConv::C);
    
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(module->getContext(), "entry", function);
    builder->SetInsertPoint(BB);
    
    ObjcCodeGenFunction(2, std::string("objcjs_main"), module);
  
    //TODO : pass arguments from main
    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(ObjcNullPointer());
    ArgsV.push_back(ObjcNullPointer());
    
    builder->CreateCall(module->getFunction("objcjs_main"), ArgsV);

    auto zeroInt = llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, 0));
    builder->CreateRet(zeroInt);
    builder->saveAndClearIP();
    return function;
}

llvm::Function *ObjcCodeGenModuleInit(llvm::IRBuilder<>*builder,
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


void SetModuleCtor(llvm::Module *module, llvm::Function *cTor){
    llvm::Function* func_init = module->getFunction("objcjs_module_init");
    assert(!func_init && "module already has a ctor");
    
    std::vector<llvm::Type*>FuncTy_args;
    llvm::FunctionType* FuncTy = llvm::FunctionType::get(
                                                         llvm::Type::getVoidTy(module->getContext()),
                                                         FuncTy_args,
                                                         false);
    func_init = llvm::Function::Create(
                                       FuncTy,
                                       llvm::GlobalValue::ExternalLinkage,
                                       "objcjs_module_init", module);
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

//TODO : explicitly specifiy varargs
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

std::string asciiStringWithV8String(v8::internal::String *string) {
    char *ascii = string->ToAsciiArray();
    return std::string(ascii);
}

llvm::Value *localStringVar(const char* data,
                            size_t len,
                            llvm::Module *module) {
//    auto exisitingString = module->getGlobalVariable(llvm::StringRef(data), true);
//    if (exisitingString){
//        return exisitingString;
//    }
    llvm::Constant *constTy = llvm::ConstantDataArray::getString(llvm::getGlobalContext(), data);
    //We are adding an extra space for the null terminator!
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

llvm::Value *localStringVar(std::string value,
                            llvm::Module *module) {
    return localStringVar(value.c_str(), value.size(), module);
}

//TODO : move to native JS
static llvm::Function *ObjcCOutPrototype(llvm::IRBuilder<>*builder,
                                         llvm::Module *module) {
    std::vector<llvm::Type*> ArgumentTypes(1, ObjcPointerTy());
    
    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(llvm::getGlobalContext()),
                                         ArgumentTypes, true);
   
    auto function = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, llvm::Twine("objcjs_cout"), module);
    
    
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(module->getContext(), "entry", function);
    builder->SetInsertPoint(BB);
    
    llvm::Function::arg_iterator argIterator = function->arg_begin();
    
    auto *alloca  = builder->CreateAlloca(ObjcPointerTy(), 0, std::string("varr"));
    builder->CreateStore(argIterator, alloca);

    llvm::Value *localVarValue = builder->CreateLoad(alloca, false, std::string("varr"));

    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(localVarValue);
    builder->CreateCall(module->getFunction("NSLog"), ArgsV);
    
    auto zero = llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(0.0));
    builder->CreateRet(zero);
    builder->saveAndClearIP();
    return function;
}

#pragma mark - Runtime calls

CGObjCJSRuntime::CGObjCJSRuntime(llvm::IRBuilder<> *builder,
                                 llvm::Module *module){
    _builder = builder;
    _module = module;
   
    _builtins.insert("objcjs_main");
    _builtins.insert("NSLog");
    
    DefExternFucntion("objc_msgSend");
    DefExternFucntion("sel_getUid");
    DefExternFucntion("objc_getClass");
    DefExternFucntion("objcjs_invoke");

    //JSRuntime
    DefExternFucntion("objcjs_defineJSFunction");
   
    ObjcCodeGenFunction(1, std::string("objcjs_increment"), _module);
    ObjcCodeGenFunction(1, std::string("objcjs_decrement"), _module);
    ObjcCodeGenFunction(3, std::string("objcjs_assignProperty"), _module);
    
    ObjcMsgSendFPret(_module);
    ObjcMallocPrototype(_module);
    ObjcNSLogPrototye(_module);
    ObjcCOutPrototype(_builder, _module);

    ObjcCodeGenMainPrototype(_builder, _module);
}

llvm::Value *CGObjCJSRuntime::newString(std::string string){
    const char *name = string.c_str();
    llvm::Value *strGlobal = localStringVar(name, string.length(), _module);
    return messageSend(classNamed("NSString"), "stringWithUTF8String:", strGlobal);
}

llvm::Value *CGObjCJSRuntime::classNamed(const char *name){
    return _builder->CreateCall(_module->getFunction("objc_getClass"),  localStringVar(std::string(name), _module), "calltmp-getclass");
}

llvm::Value *CGObjCJSRuntime::newNumber(double doubleValue){
    auto constValue = llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(doubleValue));
    return messageSend(classNamed("NSNumber"), "numberWithDouble:", constValue);
}

llvm::Value *CGObjCJSRuntime::newNumberWithLLVMValue(llvm::Value *doubleValue){
    return messageSend(classNamed("NSNumber"), "numberWithDouble:", doubleValue);
}

llvm::Value *CGObjCJSRuntime::doubleValue(llvm::Value *llvmValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), localStringVar(std::string("doubleValue"), _module), "calltmp");

    std::vector<llvm::Value*> Args;
    Args.push_back(llvmValue);
    Args.push_back(sel);

    return _builder->CreateCall(_module->getFunction("objc_msgSend_fpret"), Args, "calltmp-objc_msgSend_fpret");
}

llvm::Value *CGObjCJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *selectorName,
                                          llvm::Value *Arg) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), localStringVar(std::string(selectorName), _module), "calltmp");
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selector);
   
    if (Arg){
        Args.push_back(Arg);
    }
   
    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "calltmp-objc_msgSend");
}

llvm::Value *CGObjCJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *selectorName,
                                          std::vector<llvm::Value *>ArgsV) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), localStringVar(std::string(selectorName), _module), "calltmp");
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selector);
   
    for (int i = 0; i < ArgsV.size(); i++) {
        Args.push_back(ArgsV.at(i));
    }
  
    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "calltmp-objc_msgSend");
}

llvm::Value *CGObjCJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *selectorName) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), localStringVar(std::string(selectorName), _module), "calltmp");
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selector);
    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "calltmp-objc_msgSend");
}

llvm::Value *CGObjCJSRuntime::messageSendProperty(llvm::Value *receiver,
                                 const char *name,
                                 std::vector<llvm::Value *>ArgsV) {
   
    size_t nameLen = strlen(name);
    char *methodName = (char *)malloc((sizeof(char) * nameLen) + 2);
    memcpy(methodName, name, nameLen);
    methodName[nameLen] = ':';
    methodName[nameLen+1] = '\0';
    auto retValue = messageSend(receiver, methodName, ArgsV);
    free(methodName);
    return retValue;
}

//Sends a message to a JSFunction instance
llvm::Value *CGObjCJSRuntime::invokeJSValue(llvm::Value *instance,
                                                    std::vector<llvm::Value *>ArgsV) {
    std::vector<llvm::Value*> Args;
    Args.push_back(instance);

    for (int i = 0; i < ArgsV.size(); i++) {
        Args.push_back(ArgsV.at(i));
    }

    Args.push_back(ObjcNullPointer());
    return _builder->CreateCall(_module->getFunction("objcjs_invoke"), Args, "objcjs_invoke");
}

//Convert a llvm value to an bool
//which is represented as an int in JS land
llvm::Value *CGObjCJSRuntime::boolValue(llvm::Value *llvmValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), localStringVar(std::string("objcjs_boolValue"), _module), "calltmp");

    std::vector<llvm::Value*> Args;
    Args.push_back(llvmValue);
    Args.push_back(sel);

    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "objc_msgSend");
}

llvm::Value *CGObjCJSRuntime::defineJSFuction(const char *name,
                                              unsigned nArgs
                                                 ) {
    auto function = CGObjCJSFunction(nArgs, name, _module);
    
    auto nameAlloca = localStringVar(name, strlen(name), _module);
    
    std::vector<llvm::Value*> Args;
    Args.push_back(nameAlloca);
    Args.push_back(function);

    return llvm::CallInst::Create(_module->getFunction("objcjs_defineJSFunction"), Args, "calltmp");
}

llvm::Value *CGObjCJSRuntime::declareProperty(llvm::Value *instance,
                                              std::string name) {
    
    auto nameAlloca = localStringVar(name.c_str(), name.length(), _module);
    return messageSend(instance, "objcjs_defineProperty:", nameAlloca);
}

llvm::Value *CGObjCJSRuntime::assignProperty(llvm::Value *instance,
                                             std::string name,
                                             llvm::Value *value) {
    auto nameAlloca = localStringVar(name.c_str(), name.length(), _module);
    std::vector<llvm::Value*> Args;
    Args.push_back(instance);
    Args.push_back(nameAlloca);
    Args.push_back(value);
    
    return _builder->CreateCall(_module->getFunction("objcjs_assignProperty"), Args, "calltmp");
}

llvm::Value *CGObjCJSRuntime::declareGlobal(std::string name) {
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

bool CGObjCJSRuntime::isBuiltin(std::string name) {
    return _builtins.count(name) > 0;
}