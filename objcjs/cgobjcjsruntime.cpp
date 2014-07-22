//
//  cgobjcjsruntime.cpp
//  objcjs
//
//  Created by Jerry Marino on 7/19/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

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

//TODO : explicitly specifiy varargs
#define DefExternFucntion(name){\
{\
    std::vector<llvm::Type*> argTypes; \
    argTypes.push_back(ObjcPointerTy()); \
    llvm::FunctionType *ft = llvm::FunctionType::get(ObjcPointerTy(), argTypes, true); \
    auto function = llvm::Function::Create( \
        ft, llvm::Function::ExternalLinkage, \
        llvm::Twine(name), \
        _module );\
    function->setCallingConv(llvm::CallingConv::C); \
} \
}

static llvm::Function *ObjcMsgSendFPret(llvm::Module *module) {
        std::vector<llvm::Type*> argTypes;
    argTypes.push_back(ObjcPointerTy());
    auto doubleTy  = llvm::Type::getDoubleTy(llvm::getGlobalContext());
    llvm::FunctionType *ft = llvm::FunctionType::get(doubleTy, argTypes, true); \
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
    
    llvm::FunctionType* printf_type =
    llvm::FunctionType::get(
                            llvm::Type::getInt32Ty(module->getContext()), ArgumentTypes, true);
    
    llvm::Function *function = llvm::Function::Create(
                                                  printf_type, llvm::Function::ExternalLinkage,
                                                  llvm::Twine("NSLog"),
                                                  module
                                                  );
    function->setCallingConv(llvm::CallingConv::C);
    return function;
}

static llvm::Function *ObjcMallocPrototype(llvm::Module *module) {
    std::vector<llvm::Type*>FuncTy_7_args;
    FuncTy_7_args.push_back(llvm::IntegerType::get(module->getContext(), 64));
    llvm::FunctionType* FuncTy_7 = llvm::FunctionType::get(
                                                           /*Result=*/ObjcPointerTy(),
                                                           /*Params=*/FuncTy_7_args,
                                                           /*isVarArg=*/false);
    llvm::Function* func_malloc = module->getFunction("malloc");
    if (!func_malloc) {
        func_malloc = llvm::Function::Create(
                                             /*Type=*/FuncTy_7,
                                             /*Linkage=*/llvm::GlobalValue::ExternalLinkage,
                                             /*Name=*/"malloc", module); // (external, no body)
        func_malloc->setCallingConv(llvm::CallingConv::C);
    }
    
    return func_malloc;
}

std::string asciiStringWithV8String(v8::internal::String *string) {
    char *ascii = string->ToAsciiArray();
    return std::string(ascii);
}

llvm::Value *localStringVar(const char* data,
                            size_t len,
                            llvm::Module *module) {
    auto exisitingString = module->getGlobalVariable(llvm::StringRef(data), true);
    if (exisitingString){
        return exisitingString;
    }
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
static llvm::Function *ObjcCOutPrototype(llvm::IRBuilder<>*_builder,
                                         llvm::Module *module) {
    std::vector<llvm::Type*> ArgumentTypes(1, ObjcPointerTy());
    
    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(llvm::getGlobalContext()),
                                         ArgumentTypes, true);
   
    auto function = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, llvm::Twine("objcjs_cout"), module);
    
    
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(module->getContext(), "entry", function);
    _builder->SetInsertPoint(BB);
    
    llvm::Function::arg_iterator argIterator = function->arg_begin();
    
    llvm::IRBuilder<> Builder(&function->getEntryBlock(),
                           function->getEntryBlock().begin());
   
    auto *alloca  = Builder.CreateAlloca(ObjcPointerTy(), 0, std::string("varr"));
    _builder->CreateStore(argIterator, alloca);

    llvm::Value *localVarValue = _builder->CreateLoad(alloca, false, std::string("varr"));

    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(localVarValue);
    _builder->CreateCall(module->getFunction("NSLog"), ArgsV);
    
    auto zero = llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(0.0));
    _builder->CreateRet(zero);
    _builder->saveAndClearIP();
    return function;
}

#pragma mark - Runtime calls

CGObjCJSRuntime::CGObjCJSRuntime(llvm::IRBuilder<> *builder,
                                 llvm::Module *module){
    _builder = builder;
    _module = module;
    
    DefExternFucntion("objc_msgSend");
    DefExternFucntion("sel_getUid");
    DefExternFucntion("objc_getClass");
    DefExternFucntion("objcjs_invoke");

    //JSRuntime
    DefExternFucntion("defineJSFunction");
    
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

    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(llvmValue);
    ArgsV.push_back(sel);

    return _builder->CreateCall(_module->getFunction("objc_msgSend_fpret"), ArgsV, "calltmp-objc_msgSend_fpret");
}

llvm::Value *CGObjCJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *selectorName,
                                          llvm::Value *Arg) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), localStringVar(std::string(selectorName), _module), "calltmp");
    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(receiver);
    ArgsV.push_back(selector);
   
    if (Arg){
        ArgsV.push_back(Arg);
    }
   
    return _builder->CreateCall(_module->getFunction("objc_msgSend"), ArgsV, "calltmp-objc_msgSend");
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

    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(llvmValue);
    ArgsV.push_back(sel);

    return _builder->CreateCall(_module->getFunction("objc_msgSend"), ArgsV, "objc_msgSend");
}

llvm::Value *CGObjCJSRuntime::defineJSFuction(const char *name,
                                              unsigned nArgs
                                                 ) {
    auto function = CGObjCJSFunction(nArgs, name, _module);
    
    auto nameAlloca = localStringVar(name, strlen(name), _module);
    
    std::vector<llvm::Value*> Args;
    Args.push_back(nameAlloca);
    Args.push_back(function);

    return llvm::CallInst::Create(_module->getFunction("defineJSFunction"), Args, "calltmp");
}

