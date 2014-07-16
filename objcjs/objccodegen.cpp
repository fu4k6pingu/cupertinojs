//
//  codegen.cpp
//  objcjs
//
//  Created by Jerry Marino on 7/6/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include "objccodegen.h"
#include <llvm-c/Core.h>
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "src/scopes.h"
#include "string.h"
#include "llvm/Support/CFG.h"

#define DEBUG 1
#define ILOG(A, ...) if (DEBUG) printf(A,##__VA_ARGS__);

using namespace v8::internal;

static auto _ObjcPointerTy = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);
static llvm::Type *ObjcPointerTy(){
    return _ObjcPointerTy;
}

static llvm::Value *ObjcNullPointer(){
    auto ty = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);
    return llvm::ConstantPointerNull::get(ty);
}

static std::string stringFromV8AstRawString(const AstRawString *raw){
    std::string str;
    size_t size = raw->length();
    const unsigned char *data = raw->raw_data();
    for (int i = 0; i < size; i++) {
        str += data[i];
    }
    return str;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *Function,
                                          const std::string &VarName) {
    llvm::IRBuilder<> Builder(&Function->getEntryBlock(),
                     Function->getEntryBlock().begin());
    return Builder.CreateAlloca(ObjcPointerTy(), 0, VarName.c_str());
}

static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *Function,
                                                const std::string &VarName,
                                                llvm::Type *ty) {
    
    llvm::IRBuilder<> Builder(&Function->getEntryBlock(),
                     Function->getEntryBlock().begin());
    return Builder.CreateAlloca(ty, 0, VarName.c_str());
}

static llvm::Function *ObjcCodeGenFunction(size_t num_params, std::string name, llvm::Module *mod){
    std::vector<llvm::Type*> Params(num_params, ObjcPointerTy());
    llvm::FunctionType *FT = llvm::FunctionType::get(ObjcPointerTy(), Params, false);
    return llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, mod);
}
static std::string CMD_NAME("__cmd__");

static llvm::Function *ObjcCodeGenJSFunction(size_t num_params, std::string name, llvm::Module *mod){
    std::vector<llvm::Type*> Params;
    Params.push_back(ObjcPointerTy());
    Params.push_back(ObjcPointerTy());
//    auto type = llvm::ArrayType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), CMD_NAME.length() + 1);
//    Params.push_back(type);
    
    for (int i = 0; i < num_params; i++) {
        Params.push_back(ObjcPointerTy());
    }
    
    llvm::FunctionType *FT = llvm::FunctionType::get(ObjcPointerTy(), Params, false);
    return llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, mod);
}

static llvm::Function *ObjcCodeGenMainPrototype(llvm::IRBuilder<>*builder, llvm::Module *module){
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

static llvm::Function *ObjcMsgSendFPret(llvm::Module *module){
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

//TODO : support var args
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

static llvm::Function *ObjcMallocPrototype(llvm::Module *module){
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

std::string asciiStringWithV8String(String *string) {
    char *ascii = string->ToAsciiArray();
    return std::string(ascii);
}

llvm::Value *llvmNewLocalStringVar(const char* data, size_t len, llvm::Module *module)
{
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

llvm::Value *llvmNewLocalStringVar(std::string value, llvm::Module *module){
    std::string *name = new std::string(value);
    return llvmNewLocalStringVar(name->c_str(), name->size(), module);
}

//TODO : move to native JS
static llvm::Function *ObjcCOutPrototype(llvm::IRBuilder<>*_builder, llvm::Module *module){
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

ObjCCodeGen::ObjCCodeGen(Zone *zone){
    InitializeAstVisitor(zone);
    llvm::LLVMContext &Context = llvm::getGlobalContext();
    _builder = new llvm::IRBuilder<> (Context);
    _module = new llvm::Module("jit", Context);
    //TODO : is this needed?
    _module->setDataLayout("e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128");
    _module->setTargetTriple("x86_64-apple-macosx10.9.0");
    _context = NULL;
    
    DefExternFucntion("objc_msgSend");
    DefExternFucntion("sel_getUid");
    DefExternFucntion("objc_getClass");

    //JSRuntime
    DefExternFucntion("defineJSFunction");
    
    ObjcMsgSendFPret(_module);
    ObjcMallocPrototype(_module);
    ObjcNSLogPrototye(_module);
    ObjcCOutPrototype(_builder, _module);
    ObjcCOutPrototype(_builder, _module);

    ObjcCodeGenMainPrototype(_builder, _module);
}

/// CreateArgumentAllocas - Create an alloca for each argument and register the
/// argument in the symbol table so that references to it will succeed.
void ObjCCodeGen::CreateArgumentAllocas(llvm::Function *F, v8::internal::Scope* scope) {
    llvm::Function::arg_iterator AI = F->arg_begin();
    int num_params = scope->num_parameters();
    for (unsigned Idx = 0, e = num_params; Idx != e; ++Idx, ++AI) {
        // Create an alloca for this variable.
        Variable *param = scope->parameter(Idx);
        std::string str = stringFromV8AstRawString(param->raw_name());
        llvm::AllocaInst *alloca = CreateEntryBlockAlloca(F, str);
        
        // Store the initial value into the alloca.
        _builder->CreateStore(AI, alloca);
        _context->setValue(str, alloca);
    }
}

/// CreateArgumentAllocas - Create an alloca for each argument and register the
/// argument in the symbol table so that references to it will succeed.
void ObjCCodeGen::CreateJSArgumentAllocas(llvm::Function *F, v8::internal::Scope* scope) {
    llvm::Function::arg_iterator AI = F->arg_begin();
    llvm::Value *argSelf = AI++;
    int num_params = scope->num_parameters();
   
    std::string allocaThisName("__this");
    llvmNewLocalStringVar(allocaThisName, _module);
    llvm::AllocaInst *allocaThis = CreateEntryBlockAlloca(F, allocaThisName);
    _builder->CreateStore(argSelf, allocaThis);
    _context->setValue(allocaThisName, allocaThis);
   
    auto cmdVal = llvmNewLocalStringVar(CMD_NAME, _module);
    //We are adding an extra space for the null terminator!
//    auto cmdType = llvm::ArrayType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), CMD_NAME.length() + 1);
    
    llvm::AllocaInst *allocaCMD = CreateEntryBlockAlloca(F, CMD_NAME, ObjcPointerTy());
    llvm::Value *argCMD = AI++;
    _builder->CreateStore(argCMD, allocaCMD);
    _context->setValue(CMD_NAME, allocaCMD);
    
    for (unsigned Idx = 0, e = num_params; Idx != e; ++Idx, ++AI) {
        // Create an alloca for this variable.
        int i = Idx;
        Variable *param = scope->parameter(i);
        std::string str = stringFromV8AstRawString(param->raw_name());
        llvm::AllocaInst *alloca = CreateEntryBlockAlloca(F, str);
        
        // Store the initial value into the alloca.
        _builder->CreateStore(AI, alloca);
        _context->setValue(str, alloca);
    }
    
    
}

void ObjCCodeGen::dump(){
    _module->dump();
}

void ObjCCodeGen::VisitVariableDeclaration(v8::internal::VariableDeclaration* node) {
    //TODO : Enter into symbol table with scope..
    std::string str = stringFromV8AstRawString(node->proxy()->raw_name());
    llvm::Function *f = _builder->GetInsertBlock()->getParent();
    llvm::AllocaInst *alloca = CreateEntryBlockAlloca(f, str);
    _context->setValue(str, alloca);

    VariableProxy *var = node->proxy();
    Visit(var);
}

void ObjCCodeGen::VisitFunctionDeclaration(v8::internal::FunctionDeclaration* node) {
    std::string name = stringFromV8AstRawString(node->fun()->raw_name());
    if (!name.length()){
        ILOG("TODO: support unnamed functions");
    }
    
    //VisitFunctionLiteral
    VisitStartAccumulation(node->fun());
    //Ends scope
    EndAccumulation();
}

#pragma mark - Modules

void ObjCCodeGen::VisitModuleDeclaration(ModuleDeclaration* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitImportDeclaration(ImportDeclaration* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitExportDeclaration(ExportDeclaration* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitModuleLiteral(ModuleLiteral* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitModuleVariable(ModuleVariable* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitModulePath(ModulePath* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitModuleUrl(ModuleUrl* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitModuleStatement(ModuleStatement* node) {
    UNIMPLEMENTED();
}

#pragma mark - Statements

void ObjCCodeGen::VisitExpressionStatement(ExpressionStatement* node) {
    Visit(node->expression());
}

void ObjCCodeGen::VisitEmptyStatement(EmptyStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitIfStatement(IfStatement* node) {
    CGIfStatement(node, true);
}

void ObjCCodeGen::CGIfStatement(IfStatement *node, bool flag){
    Visit(node->condition());
    llvm::Value *condV = PopContext();
    if (!condV) {
        return;
    }

    // Convert condition to a bool by comparing equal to NullPointer.
    // in JS land bools are represented as ints
    // so comparing a NSDoubleNumber that is 0.0 will be incorrect in JS
    // so get the int value
    condV = _builder->CreateICmpEQ(boolValue(condV), ObjcNullPointer(), "ifcond");
    
    auto function = _builder->GetInsertBlock()->getParent();
    
    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "then", function);
    llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "else");
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "ifcont");
    
    _builder->CreateCondBr(condV, elseBB, thenBB);
    
    // Emit then value.
    _builder->SetInsertPoint(thenBB);

    llvm::Value *thenV;
    if (node->HasThenStatement()){
        //TODO : new scope
        Visit(node->then_statement());
        thenV = PopContext();
        if (!thenV){
            thenV = ObjcNullPointer();
        }
    } else {
        //no op
        thenV = ObjcNullPointer();
    }
    
    _builder->CreateBr(mergeBB);
    // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
    thenBB = _builder->GetInsertBlock();
    
    // Emit else block.
    function->getBasicBlockList().push_back(elseBB);
    _builder->SetInsertPoint(elseBB);
    

    llvm::Value *elseV;
    if (node->HasElseStatement()) {
        //TODO : new scope
        Visit(node->else_statement());
        elseV = PopContext();
        if (!elseV) {
            elseV = ObjcNullPointer();
        }
    } else {
        //no op
        elseV = ObjcNullPointer();
    }

    _builder->CreateBr(mergeBB);
    // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
    elseBB = _builder->GetInsertBlock();
    
    // Emit merge block.
    function->getBasicBlockList().push_back(mergeBB);
    _builder->SetInsertPoint(mergeBB);
   
  
    llvm::PHINode *ph = _builder->CreatePHI(
                                      ObjcPointerTy(),
                                      2, "condphi");
    ph->addIncoming(thenV, thenBB);
    ph->addIncoming(elseV, elseBB);
}


void ObjCCodeGen::VisitContinueStatement(ContinueStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitBreakStatement(BreakStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitReturnStatement(ReturnStatement* node) {
    Visit(node->expression());
    llvm::BasicBlock *block = _builder->GetInsertBlock();
   
    auto blockName = block->getName();
    if (blockName.startswith(llvm::StringRef("then")) ||
        blockName.startswith(llvm::StringRef("else"))
        ){
        //In an if statement, the PHINode needs to have
        //the type 0 assigned to it!
        llvm::AllocaInst *alloca = _context->valueForKey(STR("SET_RET_ALLOCA"));
        _builder->CreateStore(PopContext(), alloca);
        //TODO : remove this!
        PushValueToContext(ObjcNullPointer());
        return;
    }

    llvm::Function *currentFunction = block->getParent();
    assert(currentFunction->getReturnType() == ObjcPointerTy() && _context->size());
    llvm::AllocaInst *alloca = _context->valueForKey(STR("DEFAULT_RET_ALLOCA"));
    auto retValue = PopContext();
    assert(retValue && "requires return value");
    _builder->CreateStore(retValue, alloca);
}


void ObjCCodeGen::VisitWithStatement(WithStatement* node) {
    UNIMPLEMENTED(); //Deprecated
}


#pragma mark - Switch

void ObjCCodeGen::VisitSwitchStatement(SwitchStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitCaseClause(CaseClause* clause) {
    UNIMPLEMENTED();
}

#pragma mark - Loops

void ObjCCodeGen::VisitDoWhileStatement(DoWhileStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitWhileStatement(WhileStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitForStatement(ForStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitForInStatement(ForInStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitForOfStatement(ForOfStatement* node) {
    UNIMPLEMENTED();
}

#pragma mark - Try
void ObjCCodeGen::VisitTryCatchStatement(TryCatchStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitTryFinallyStatement(TryFinallyStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitDebuggerStatement(DebuggerStatement* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitFunctionLiteral(v8::internal::FunctionLiteral* node) {
    auto name = stringFromV8AstRawString(node->raw_name());
    if (!name.length()){
        ILOG("TODO: support unnamed functions");
        VisitDeclarations(node->scope()->declarations());
        VisitStatements(node->body());
        
        return;
    }
    
    v8::internal::Scope *scope = node->scope();
    int num_params = scope->num_parameters();
    auto function = _module->getFunction(name) ? _module->getFunction(name) : ObjcCodeGenJSFunction(num_params, name, _module);
  
   
    if (name == std::string("larryf")){
//        llvm::Constant *fPointer = llvm::ConstantExpr::getCast(llvm::Instruction::BitCast, function, function->getType());
        //Add define JSFunction to front of main
        llvm::Function *main = _module->getFunction("main");
        auto nameAlloca = llvmNewLocalStringVar(name.c_str(), name.length(), _module);
        std::vector<llvm::Value*> ArgsV;
        ArgsV.push_back(nameAlloca);
        ArgsV.push_back(function);
        auto call = _builder->CreateCall(_module->getFunction("defineJSFunction"), ArgsV, "calltmp");
        llvm::BasicBlock *mainBB = &main->getBasicBlockList().front();
        mainBB->getInstList().push_front(call);
    }
    
    // Create a new basic block to start insertion into.
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", function);
    _builder->SetInsertPoint(BB);
   
    if (num_params){
        bool isJSFunction = !(name == std::string("objcjs_main") || name == std::string("NSLog"));
        if (isJSFunction) {
            CreateJSArgumentAllocas(function, node->scope());
        } else {
            CreateArgumentAllocas(function, node->scope());
        }
    }
   
    //Return types
//    //TODO : don't malloc a return sentenenial everytime
    auto sentenialReturnAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, std::string("sentential"));
    llvm::ConstantInt* const_int64_10 = llvm::ConstantInt::get(_module->getContext(), llvm::APInt(64, llvm::StringRef("8"), 10));
    auto retPtr = _builder->CreateCall(_module->getFunction("malloc"), const_int64_10, "calltmp");
    _builder->CreateStore(retPtr, sentenialReturnAlloca);
    auto sentenialReturnValue = _builder->CreateLoad(sentenialReturnAlloca);
    
    
    //end ret alloca is the return value at the end of a function
    auto endRetAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, std::string("endret"));
    //TODO : instead of returnig a null pointer, return a 'undefined' sentenial
    _builder->CreateStore(ObjcNullPointer(), endRetAlloca);
    _context->setValue(STR("DEFAULT_RET_ALLOCA") , endRetAlloca);
    
    auto retAlloca =  _builder->CreateAlloca(ObjcPointerTy(), 0, std::string("ret"));
    _builder->CreateStore(sentenialReturnValue, retAlloca);
    _context->setValue(STR("SET_RET_ALLOCA"), retAlloca);
    
    VisitDeclarations(node->scope()->declarations());
    VisitStatements(node->body());

    assert(function->getReturnType() == ObjcPointerTy() && "all functions return pointers");
    
    auto retValue = _builder->CreateLoad(retAlloca, "retalloca");
   
    //If the return value was set, then return what it was set to
    auto condV = _builder->CreateICmpEQ(retValue, sentenialReturnValue, "ifsetreturn");
   
    auto setRetBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "setret", function);
    auto defaultRetBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "endret");
    _builder->CreateCondBr(condV, defaultRetBB, setRetBB);
    
    function->getBasicBlockList().push_back(defaultRetBB);
    _builder->SetInsertPoint(defaultRetBB);
    auto endRetValue = _builder->CreateLoad(_context->valueForKey(STR("DEFAULT_RET_ALLOCA")), "endretalloca");
    _builder->CreateRet(endRetValue);
    
    _builder->SetInsertPoint(setRetBB);
    _builder->CreateRet(_builder->CreateLoad(retAlloca, "retallocaend"));
  
    _builder->saveAndClearIP();
    if (_context) {
        std::cout << "Context size:" << _context->size();
    }
}

void ObjCCodeGen::VisitNativeFunctionLiteral(NativeFunctionLiteral* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitConditional(Conditional* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitLiteral(class Literal* node) {
    CGLiteral(node->value(), true);
}


#pragma mark - Runtime calls

llvm::Value *ObjCCodeGen::newString(std::string string){
    const char *name = string.c_str();
    llvm::Value *strGlobal = llvmNewLocalStringVar(name, string.length(), _module);
    return messageSend(classNamed("NSString"), "stringWithUTF8String:", strGlobal);
}

llvm::Value *ObjCCodeGen::classNamed(const char *name){
    return _builder->CreateCall(_module->getFunction("objc_getClass"),  llvmNewLocalStringVar(std::string(name), _module), "calltmp-getclass");
}

llvm::Value *ObjCCodeGen::newNumber(double doubleValue){
    auto constValue = llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(doubleValue));
    return messageSend(classNamed("NSNumber"), "numberWithDouble:", constValue);
}

llvm::Value *ObjCCodeGen::newNumberWithLLVMValue(llvm::Value *doubleValue){
    return messageSend(classNamed("NSNumber"), "numberWithDouble:", doubleValue);
}

llvm::Value *ObjCCodeGen::doubleValue(llvm::Value *llvmValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string("doubleValue"), _module), "calltmp");

    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(llvmValue);
    ArgsV.push_back(sel);

    llvm::Value *value = _builder->CreateCall(_module->getFunction("objc_msgSend_fpret"), ArgsV, "calltmp-objc_msgSend_fpret");
    return value;
}

llvm::Value *ObjCCodeGen::messageSend(llvm::Value *receiver, const char *selectorName, llvm::Value *Arg) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string(selectorName), _module), "calltmp");
    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(receiver);
    ArgsV.push_back(selector);
    ArgsV.push_back(Arg);
    auto resultValue = _builder->CreateCall(_module->getFunction("objc_msgSend"), ArgsV, "calltmp-objc_msgSend");
    return resultValue;
}

llvm::Value *ObjCCodeGen::messageSend(llvm::Value *receiver, const char *selectorName, std::vector<llvm::Value *>ArgsV) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string(selectorName), _module), "calltmp");
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selector);
    assert(ArgsV.size() == 1);
    Args.push_back(ArgsV.back());

    auto resultValue = _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "calltmp-objc_msgSend");
    return resultValue;
}

llvm::Value *ObjCCodeGen::messageSend(llvm::Value *receiver, const char *selectorName) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string(selectorName), _module), "calltmp");
    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(receiver);
    ArgsV.push_back(selector);
    auto resultValue = _builder->CreateCall(_module->getFunction("objc_msgSend"), ArgsV, "objc_msgSend");
    return resultValue;
}

llvm::Value *ObjCCodeGen::messageSendJSFunction(llvm::Value *instance, std::vector<llvm::Value *>ArgsV) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string("body:"), _module), "calltmp");
    std::vector<llvm::Value*> Args;
    Args.push_back(instance);
    Args.push_back(selector);
    assert(ArgsV.size() == 1);
    Args.push_back(ArgsV.back());
    
//    auto resultValue = _builder->CreateCall(_module->getFunction("objc_msgSend"), ArgsV, "call-tmpobjc_msgSend");
    unsigned Idx = 0;
    auto function = _module->getFunction(llvm::StringRef("larryf"));
    for (llvm::Function::arg_iterator AI = function->arg_begin(); Idx != ArgsV.size();
         ++AI, ++Idx){
        llvm::Value *arg = Args.at(Idx);
        
        // check param types
        llvm::Type *argTy = arg->getType();
        llvm::Type *paramTy = AI->getType();
        assert(argTy == paramTy);
        //
    }
    
    auto resultValue = _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "objc_msgSend");
//    auto resultValue = _builder->CreateCall(function, Args);
    return resultValue;
}

//Convert a llvm value to an bool
//which is represented as an int in JS land
llvm::Value *ObjCCodeGen::boolValue(llvm::Value *llvmValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string("intValue"), _module), "calltmp");

    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(llvmValue);
    ArgsV.push_back(sel);

    llvm::Value *value = _builder->CreateCall(_module->getFunction("objc_msgSend"), ArgsV, "objc_msgSend");
    return value;
}

#pragma mark - Literals

llvm::Value *ObjCCodeGen::CGLiteral(Handle<Object> value, bool push) {
    Object* object = *value;
    llvm::Value *lvalue = NULL;
    if (object->IsString()) {
        String* string = String::cast(object);
        auto name = asciiStringWithV8String(string);
        if (name.length()){
            lvalue = newString(name);
        }
        
        ILOG("STRING literal: %s", name.c_str());
    } else if (object->IsNull()) {
        lvalue = newNumber(0);
        ILOG("null literal");
        lvalue = ObjcNullPointer();
    } else if (object->IsTrue()) {
        ILOG("true literal");
        lvalue = newNumber(1);
    } else if (object->IsFalse()) {
        ILOG("false literal");
        lvalue = newNumber(0);
    } else if (object->IsUndefined()) {
        ILOG("undefined");
    } else if (object->IsNumber()) {
        lvalue = newNumber(object->Number());
        ILOG("NUMBER value %g", object->Number());
    } else if (object->IsJSObject()) {
        assert(0 && "Not implmeneted");
        // regular expression
        if (object->IsJSFunction()) {
            ILOG("JS-Function");
        } else if (object->IsJSArray()) {
            ILOG("JS-array[%u]", JSArray::cast(object)->length());
        } else if (object->IsJSObject()) {
            ILOG("JS-Object");
        } else {
            ILOG("?UNKNOWN?");
        }
    } else if (object->IsFixedArray()) {
        ILOG("FixedArray");
    } else {
        ILOG("<unknown literal %p>", object);
    }
   
    if (push && lvalue) {
        PushValueToContext(lvalue);
    }
   
    return NULL;
}

void ObjCCodeGen::VisitRegExpLiteral(RegExpLiteral* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitObjectLiteral(ObjectLiteral* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitArrayLiteral(ArrayLiteral* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitVariableProxy(VariableProxy* node) {
    EmitVariableLoad(node);
}

//TODO : checkout full-codegen-x86.cc
void ObjCCodeGen::VisitAssignment(Assignment* node) {
    ASSERT(node->target()->IsValidReferenceExpression());

    enum LhsKind { VARIABLE, NAMED_PROPERTY, KEYED_PROPERTY };
    LhsKind assign_type = VARIABLE;
    Property* property = node->target()->AsProperty();
    
    if (property != NULL) {
        assign_type = (property->key()->IsPropertyName())
        ? NAMED_PROPERTY
        : KEYED_PROPERTY;
    }
    switch (assign_type) {
        case VARIABLE:{
            //Do nothing
            break;
        }
        case KEYED_PROPERTY:{
            
        }
        case NAMED_PROPERTY:{
            
        }
    }
   
//TODO :
// For compound assignments we need another deoptimization point after the
// variable/property load.
//    if (node->is_compound()) {
//        //            AccumulatorValueContext context(this);
//        switch (assign_type) {
//            case VARIABLE:
//                EmitVariableLoad(node->target()->AsVariableProxy());
//                //                    PrepareForBailout(expr->target(), TOS_REG);
//                break;
//            case NAMED_PROPERTY:
//                //                    EmitNamedPropertyLoad(property);
//                //                    PrepareForBailoutForId(property->LoadId(), TOS_REG);
//                break;
//            case KEYED_PROPERTY:
//                //                    EmitKeyedPropertyLoad(property);
//                //                    PrepareForBailoutForId(property->LoadId(), TOS_REG);
//                break;
//        }
//        
//        Token::Value op = node->binary_op();
//        //        assert(op == v8::internal::Token::ADD);
//        //        __ Push(rax);  // Left operand goes on the stack.
//        VisitStartStackAccumulation(node->value());
//       //            EmitBinaryOp(expr->binary_operation(), op, mode);
//    }
   
    //TODO : new scope
    Visit(node->value());
    
    // Store the value.
    switch (assign_type) {
        case VARIABLE: {
            VariableProxy *target = (VariableProxy *)node->target();
            assert(target);
            std::string targetName = stringFromV8AstRawString(target->raw_name());
            //TODO : respect scopes!
            llvm::AllocaInst *alloca;
            if (!_context->valueForKey(targetName)){
                alloca = _builder->CreateAlloca(ObjcPointerTy(), 0, targetName);
                _context->setValue(targetName, alloca);
            } else {
                alloca = _context->valueForKey(targetName);
            }
           
//            EmitVariableAssignment(node->target()->AsVariableProxy()->var(), node->op());
          
            llvm::Value *value = PopContext();
            if (value) {
//                _builder->CreateLoad(alloca);
                _builder->CreateStore(value, alloca);
                //TODO : remove because it is unneeded?
                //An assignment returns the value 0
                PushValueToContext(ObjcNullPointer());
            } else {
                assert(0 && "TODO: not implemented!");
            }
            
            break;
            
        } case NAMED_PROPERTY: {
            //            EmitNamedPropertyAssignment(expr);
            break;
        }
        case KEYED_PROPERTY: {
            //            EmitKeyedPropertyAssignment(expr);
            break;
        }
    }
}
void ObjCCodeGen::EmitBinaryOp(BinaryOperation* expr, Token::Value op){
//TODO:
}

void ObjCCodeGen::EmitVariableAssignment(Variable* var, Token::Value op) {
//TODO:
}

void ObjCCodeGen::EmitVariableLoad(VariableProxy* node) {
    std::string variableName = stringFromV8AstRawString(node->raw_name());
    auto varValue = _context->valueForKey(variableName);
    PushValueToContext(_builder->CreateLoad(varValue, variableName));
}

void ObjCCodeGen::VisitYield(Yield* node) {
    UNIMPLEMENTED(); //Deprecated
}

void ObjCCodeGen::VisitThrow(Throw* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitProperty(Property* node) {
    UNIMPLEMENTED();
}

Call::CallType GetCallType(Call*call, Isolate* isolate) {
    VariableProxy* proxy = call->expression()->AsVariableProxy();
    if (proxy != NULL && proxy->var()) {
        
        if (proxy->var()->IsUnallocated()) {
            return Call::GLOBAL_CALL;
        } else if (proxy->var()->IsLookupSlot()) {
            return Call::LOOKUP_SLOT_CALL;
        }
    }
    
    Property* property = call->expression()->AsProperty();
    return property != NULL ? Call::PROPERTY_CALL : Call::OTHER_CALL;
}

void ObjCCodeGen::VisitCall(Call* node) {
    Expression *callee = node->expression();
    Call::CallType callType = GetCallType(node, isolate());
   
    std::string name;
    
    if (callType == Call::GLOBAL_CALL) {
        assert(0 == "unimplemented");
    } else if (callType == Call::LOOKUP_SLOT_CALL) {
        VariableProxy* proxy = callee->AsVariableProxy();
         name = stringFromV8AstRawString(proxy->raw_name());
    } else if (callType == Call::OTHER_CALL){
        VariableProxy* proxy = callee->AsVariableProxy();
        name = stringFromV8AstRawString(proxy->raw_name());
        
    } else {
        VariableProxy* proxy = callee->AsVariableProxy();
        name = stringFromV8AstRawString(proxy->raw_name());
    }
   
    auto *calleeF = _module->getFunction(name);
    
    if (calleeF == 0){
        //TODO: this would call an objc or c function in the future
        assert(0 && "Unknown function referenced");
        return;
    }

    ZoneList<Expression*>* args = node->arguments();
   
    for (int i = 0; i <args->length(); i++) {
        //TODO : this would likely retain the values
        Visit(args->at(i));
    }
   
    if (_context->size() == 0) {
        return;
    }
   
    if (!calleeF->isVarArg()) {
        assert(calleeF->arg_size() == args->length() + 2 && "Unknown function referenced: airity mismatch");
    }
    
    std::vector<llvm::Value *> finalArgs;
    unsigned Idx = 0;
    for (llvm::Function::arg_iterator AI = calleeF->arg_begin(); Idx != args->length();
         ++AI, ++Idx){
        llvm::Value *arg = PopContext();
    
        // check param types
//        llvm::Type *argTy = arg->getType();
//        llvm::Type *paramTy = AI->getType();
//        assert(argTy == paramTy);
        //
        finalArgs.push_back(arg);
    }
    std::reverse(finalArgs.begin(), finalArgs.end());

    if (_context) {
        std::cout << '\n' << __PRETTY_FUNCTION__ << "Context size:" << _context->size();
    }

    bool isJSFunction = !(name == std::string("objcjs_main") || name == std::string("NSLog"));
    if (isJSFunction) {
        //Create a new instance and invoke the body
        llvm::Value *instance = messageSend(classNamed(name.c_str()), "new");
        
        auto value = messageSendJSFunction(instance, finalArgs);
        PushValueToContext(value);
    } else {
        PushValueToContext(_builder->CreateCall(calleeF, finalArgs, "calltmp"));
    }
}

void ObjCCodeGen::VisitCallNew(CallNew* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitCallRuntime(CallRuntime* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitUnaryOperation(UnaryOperation* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitCountOperation(CountOperation* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitBinaryOperation(BinaryOperation* expr) {
  switch (expr->op()) {
    case Token::COMMA:
      return VisitComma(expr);
    case Token::OR:
    case Token::AND:
      return VisitLogicalExpression(expr);
    default:
      return VisitArithmeticExpression(expr);
  }
}

void ObjCCodeGen::VisitComma(BinaryOperation* expr) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitLogicalExpression(BinaryOperation* expr) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitArithmeticExpression(BinaryOperation* expr) {
    Expression *left = expr->left();
    Expression *right = expr->right();
  
    //TODO : stack scope
    Visit(left);
    auto lhs = PopContext();
    
    //TODO : stack scope
    Visit(right);
    auto rhs = PopContext();
   
    llvm::Value *result = NULL;
    auto op = expr->op();
    switch (op) {
        case v8::internal::Token::ADD : {
            llvm::Value *floatResult = _builder->CreateFAdd(doubleValue(lhs), doubleValue(rhs), "addtmp");
            result = newNumberWithLLVMValue(floatResult);
            break;
        }
        case v8::internal::Token::SUB : {
            llvm::Value *floatResult = _builder->CreateFSub(doubleValue(lhs), doubleValue(rhs), "subtmp");
            result = newNumberWithLLVMValue(floatResult);
            break;
        }
        case v8::internal::Token::MUL : {
            llvm::Value *floatResult = _builder->CreateFMul(doubleValue(lhs), doubleValue(rhs), "multmp");
            result = newNumberWithLLVMValue(floatResult);
            break;   
        }
        case v8::internal::Token::DIV : {
            llvm::Value *floatResult = _builder->CreateFDiv(doubleValue(lhs), doubleValue(rhs), "divtmp");
            result = newNumberWithLLVMValue(floatResult);
            break;
        }

        default: break;
    }
    
    PushValueToContext(result);
}

// Check for the form (%_ClassOf(foo) === 'BarClass').
static bool IsClassOfTest(CompareOperation* expr) {
    if (expr->op() != Token::EQ_STRICT) return false;
    CallRuntime* call = expr->left()->AsCallRuntime();
    if (call == NULL) return false;
    Literal* literal = expr->right()->AsLiteral();
    if (literal == NULL) return false;
    if (!literal->value()->IsString()) return false;
    if (!call->name()->IsOneByteEqualTo(STATIC_ASCII_VECTOR("_ClassOf"))) {
        return false;
    }
    ASSERT(call->arguments()->length() == 1);
    return true;
}
 
void ObjCCodeGen::VisitCompareOperation(CompareOperation* expr) {
      ASSERT(!HasStackOverflow());
    llvm::Value *resultValue = NULL;
//    ASSERT(current_block() != NULL);
//    ASSERT(current_block()->HasPredecessor());
    
//    if (!FLAG_hydrogen_track_positions) SetSourcePosition(expr->position());
    
    // Check for a few fast cases. The AST visiting behavior must be in sync
    // with the full codegen: We don't push both left and right values onto
    // the expression stack when one side is a special-case literal.
    Expression* sub_expr = NULL;
    Handle<String> check;
    if (expr->IsLiteralCompareTypeof(&sub_expr, &check)) {
        UNIMPLEMENTED();
//        return HandleLiteralCompareTypeof(expr, sub_expr, check);
    }
    if (expr->IsLiteralCompareUndefined(&sub_expr, isolate())) {
        UNIMPLEMENTED();
//        return HandleLiteralCompareNil(expr, sub_expr, kUndefinedValue);
    }
    if (expr->IsLiteralCompareNull(&sub_expr)) {
        UNIMPLEMENTED();
//        return HandleLiteralCompareNil(expr, sub_expr, kNullValue);
    }
    
    if (IsClassOfTest(expr)) {
        UNIMPLEMENTED();
//        CallRuntime* call = expr->left()->AsCallRuntime();
//        ASSERT(call->arguments()->length() == 1);
//        CHECK_ALIVE(VisitForValue(call->arguments()->at(0)));
//        HValue* value = Pop();
//        Literal* literal = expr->right()->AsLiteral();
//        Handle<String> rhs = Handle<String>::cast(literal->value());
//        HClassOfTestAndBranch* instr = New<HClassOfTestAndBranch>(value, rhs);
//        return ast_context()->ReturnControl(instr, expr->id());
    }

    
//    if (IsLiteralCompareBool(isolate(), left, op, right)) {
//    UNIMPLEMENTED();
//    }

    
    Visit(expr->left());
    auto left = PopContext();
    Visit(expr->right());
    auto right = PopContext();
    
    Token::Value op = expr->op();
    Handle<JSFunction> target = Handle<JSFunction>::null();
    if (op == Token::INSTANCEOF) {
        // Check to see if the rhs of the instanceof is a global function not
        // residing in new space. If it is we assume that the function will stay the
        // same.
        
        // If the target is not null we have found a known global function that is
        // assumed to stay the same for this instanceof.
        if (target.is_null()) {
            UNIMPLEMENTED();
        } else {
            UNIMPLEMENTED();
        }
        
        // Code below assumes that we don't fall through.
        UNREACHABLE();
    } else if (op == Token::IN) {
        UNIMPLEMENTED();
    } else if (op == Token::LT) {
        resultValue = messageSend(left, "isLessThan:",  right);
    } else if (op == Token::GT){
        resultValue = messageSend(left, "isGreaterThan:",  right);
    } else if (op == Token::LTE){
        resultValue = messageSend(left, "isLessThanOrEqualTo:",  right);
    } else if (op == Token::GTE){
        resultValue = messageSend(left, "isGreaterThanOrEqualTo:",  right);
    }
    
    assert(resultValue);
    PushValueToContext(resultValue);
}

void ObjCCodeGen::VisitThisFunction(ThisFunction* node) {
    UNIMPLEMENTED();
}

void ObjCCodeGen::VisitStartAccumulation(AstNode *expr, bool extendContext) {
    CGContext *context;
    if (extendContext) {
        context = _context->Extend();
    } else {
        context = new CGContext();
    }
    
    Contexts.push_back(context);
    _context = context;
    Visit(expr);
}

void ObjCCodeGen::VisitStartAccumulation(AstNode *expr) {
    VisitStartAccumulation(expr, false);
}

void ObjCCodeGen::EndAccumulation() {
    delete _context;
    auto context = Contexts.back();
    Contexts.pop_back();
    _context = context;
}

void ObjCCodeGen::VisitStartStackAccumulation(AstNode *expr) {
    //TODO : retain values
    VisitStartAccumulation(expr, false);
}

void ObjCCodeGen::EndStackAccumulation(){
    assert(0);
    delete _context;
    auto context = Contexts.back();
    Contexts.pop_back();
    _context = context;
}

void ObjCCodeGen::PushValueToContext(llvm::Value *value) {
    _context->Push(value);
}

llvm::Value *ObjCCodeGen::PopContext() {
    return _context->Pop();
}
