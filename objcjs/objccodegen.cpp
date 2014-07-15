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

// Make the function type:  double(double,double) etc.
static llvm::Function *ObjcCodeGenFunction(size_t num_params, std::string name, llvm::Module *mod){
    std::vector<llvm::Type*> Params(num_params, ObjcPointerTy());
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
    _accumulatorContext = NULL;
    _stackAccumulatorContext = NULL;
    _context = NULL;
    _bailout = 0;
    
    DefExternFucntion("objc_msgSend");
    DefExternFucntion("sel_getUid");
    DefExternFucntion("objc_getClass");
   
    ObjcMsgSendFPret(_module);
    ObjcMallocPrototype(_module);
    ObjcNSLogPrototye(_module);
    ObjcCOutPrototype(_builder, _module);
    ObjcCodeGenMainPrototype(_builder, _module);
    ObjcCOutPrototype(_builder, _module);
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
        llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(F, str);
        
        // Store the initial value into the alloca.
        _builder->CreateStore(AI, Alloca);
        
        // Add arguments to variable symbol table.
        _namedValues[str] = Alloca;
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
    _namedValues[str] = alloca;

    VariableProxy *var = node->proxy();
    Visit(var);
}

void ObjCCodeGen::VisitFunctionDeclaration(v8::internal::FunctionDeclaration* node) {
    std::string name = stringFromV8AstRawString(node->fun()->raw_name());
    if (!name.length()){
        ILOG("TODO: support unnamed functions");
    }
    VisitFunctionLiteral(node->fun());
}


void ObjCCodeGen::VisitModuleDeclaration(ModuleDeclaration* node) {
//  Print("module ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(" = ");
//  Visit(node->module());
//  Print(";");
}


void ObjCCodeGen::VisitImportDeclaration(ImportDeclaration* node) {
//  Print("import ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(" from ");
//  Visit(node->module());
//  Print(";");
}


void ObjCCodeGen::VisitExportDeclaration(ExportDeclaration* node) {
//  Print("export ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(";");
}


void ObjCCodeGen::VisitModuleLiteral(ModuleLiteral* node) {
//  VisitBlock(node->body());
}


void ObjCCodeGen::VisitModuleVariable(ModuleVariable* node) {
//  Visit(node->proxy());
}


void ObjCCodeGen::VisitModulePath(ModulePath* node) {
//  Visit(node->module());
//  Print(".");
//  PrintLiteral(node->name(), false);
}


void ObjCCodeGen::VisitModuleUrl(ModuleUrl* node) {
//  Print("at ");
//  PrintLiteral(node->url(), true);
}


void ObjCCodeGen::VisitModuleStatement(ModuleStatement* node) {
//  Print("module ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(" ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitExpressionStatement(ExpressionStatement* node) {
    Visit(node->expression());
}


void ObjCCodeGen::VisitEmptyStatement(EmptyStatement* node) {
//    PushValueToContext(llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(0.0)));
}


void ObjCCodeGen::VisitIfStatement(IfStatement* node) {
    CGIfStatement(node, true);
}

void ObjCCodeGen::CGIfStatement(IfStatement *node, bool flag){
    VisitStartAccumulation(node->condition());
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
        VisitStartAccumulation(node->then_statement());
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
        VisitStartAccumulation(node->else_statement());
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
//  Print("continue");
//  ZoneList<const AstRawString*>* labels = node->target()->labels();
//  if (labels != NULL) {
//    Print(" ");
//    ASSERT(labels->length() > 0);  // guaranteed to have at least one entry
//    PrintLiteral(labels->at(0), false);  // any label from the list is fine
//  }
//  Print(";");
}


void ObjCCodeGen::VisitBreakStatement(BreakStatement* node) {
//  Print("break");
//  ZoneList<const AstRawString*>* labels = node->target()->labels();
//  if (labels != NULL) {
//    Print(" ");
//    ASSERT(labels->length() > 0);  // guaranteed to have at least one entry
//    PrintLiteral(labels->at(0), false);  // any label from the list is fine
//  }
//  Print(";");
}

void ObjCCodeGen::VisitReturnStatement(ReturnStatement* node) {
    VisitStartAccumulation(node->expression());
    llvm::BasicBlock *block = _builder->GetInsertBlock();
   
    auto blockName = block->getName();
    if (blockName.startswith(llvm::StringRef("then")) ||
        blockName.startswith(llvm::StringRef("else"))
        ){
        //In an if statement, the PHINode needs to have
        //the type 0 assigned to it!
        llvm::AllocaInst *alloca = _namedValues[std::string("SET_RET_ALLOCA")];
        _builder->CreateStore(PopContext(), alloca);
        //TODO : remove this!
        PushValueToContext(ObjcNullPointer());
        return;
    }

    llvm::Function *currentFunction = block->getParent();
    assert(currentFunction->getReturnType() == ObjcPointerTy() && _context->size());
    llvm::AllocaInst *alloca = _namedValues[std::string("DEFAULT_RET_ALLOCA")];
    auto retValue = PopContext();
    assert(retValue && "requires return value");
    _builder->CreateStore(retValue, alloca);
}


void ObjCCodeGen::VisitWithStatement(WithStatement* node) {
//  Print("with (");
//  Visit(node->expression());
//  Print(") ");
//  Visit(node->statement());
}


void ObjCCodeGen::VisitSwitchStatement(SwitchStatement* node) {
//  PrintLabels(node->labels());
//  Print("switch (");
//  Visit(node->tag());
//  Print(") { ");
//  ZoneList<CaseClause*>* cases = node->cases();
//  for (int i = 0; i < cases->length(); i++)
//    Visit(cases->at(i));
//  Print("}");
}


void ObjCCodeGen::VisitCaseClause(CaseClause* clause) {
//  if (clause->is_default()) {
//    Print("default");
//  } else {
//    Print("case ");
//    Visit(clause->label());
//  }
//  Print(": ");
//  PrintStatements(clause->statements());
//  if (clause->statements()->length() > 0)
//    Print(" ");
}


void ObjCCodeGen::VisitDoWhileStatement(DoWhileStatement* node) {
//  PrintLabels(node->labels());
//  Print("do ");
//  Visit(node->body());
//  Print(" while (");
//  Visit(node->cond());
//  Print(");");
}


void ObjCCodeGen::VisitWhileStatement(WhileStatement* node) {
//  PrintLabels(node->labels());
//  Print("while (");
//  Visit(node->cond());
//  Print(") ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitForStatement(ForStatement* node) {
//  PrintLabels(node->labels());
//  Print("for (");
//  if (node->init() != NULL) {
//    Visit(node->init());
//    Print(" ");
//  } else {
//    Print("; ");
//  }
//  if (node->cond() != NULL) Visit(node->cond());
//  Print("; ");
//  if (node->next() != NULL) {
//    Visit(node->next());  // prints extra ';', unfortunately
//    // to fix: should use Expression for next
//  }
//  Print(") ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitForInStatement(ForInStatement* node) {
//  PrintLabels(node->labels());
//  Print("for (");
//  Visit(node->each());
//  Print(" in ");
//  Visit(node->enumerable());
//  Print(") ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitForOfStatement(ForOfStatement* node) {
//  PrintLabels(node->labels());
//  Print("for (");
//  Visit(node->each());
//  Print(" of ");
//  Visit(node->iterable());
//  Print(") ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitTryCatchStatement(TryCatchStatement* node) {
//  Print("try ");
//  Visit(node->try_block());
//  Print(" catch (");
//  const bool quote = false;
//  PrintLiteral(node->variable()->name(), quote);
//  Print(") ");
//  Visit(node->catch_block());
}


void ObjCCodeGen::VisitTryFinallyStatement(TryFinallyStatement* node) {
//  Print("try ");
//  Visit(node->try_block());
//  Print(" finally ");
//  Visit(node->finally_block());
}


void ObjCCodeGen::VisitDebuggerStatement(DebuggerStatement* node) {
//  Print("debugger ");
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
    auto function = _module->getFunction(name) ? _module->getFunction(name) : ObjcCodeGenFunction(num_params, name, _module);
    
    // Create a new basic block to start insertion into.
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", function);
    _builder->SetInsertPoint(BB);
    
    if (num_params){
        CreateArgumentAllocas(function, node->scope());
    }
   
    //Return types
    
    //TODO : don't malloc a return sentenenial everytime
    auto sentenialReturnAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, std::string("sentential"));
    llvm::ConstantInt* const_int64_10 = llvm::ConstantInt::get(_module->getContext(), llvm::APInt(64, llvm::StringRef("8"), 10));
    auto retPtr = _builder->CreateCall(_module->getFunction("malloc"), const_int64_10, "calltmp");
    _builder->CreateStore(retPtr, sentenialReturnAlloca);
    auto sentenialReturnValue = _builder->CreateLoad(sentenialReturnAlloca);
    
    
    //end ret alloca is the return value at the end of a function
    auto endRetAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, std::string("endret"));
    //TODO : instead of returnig a null pointer, return a 'undefined' sentenial
    _builder->CreateStore(ObjcNullPointer(), endRetAlloca);
    _namedValues[std::string("DEFAULT_RET_ALLOCA")] = endRetAlloca;
   
    auto retAlloca =  _builder->CreateAlloca(ObjcPointerTy(), 0, std::string("ret"));
    _builder->CreateStore(sentenialReturnValue, retAlloca);
    _namedValues[std::string("SET_RET_ALLOCA")] = retAlloca;
    
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
    auto endRetValue = _builder->CreateLoad(_namedValues[std::string("DEFAULT_RET_ALLOCA")], "endretalloca");
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
//  Visit(node->Star        ition());
//  Print(" ? ");
//  Visit(node->then_expression());
//  Print(" : ");
//  Visit(node->else_expression());
}


void ObjCCodeGen::VisitLiteral(class Literal* node) {
    CGLiteral(node->value(), true);
}

std::string asciiStringWithV8String(String *string) {
    char *ascii = string->ToAsciiArray();
    return std::string(ascii);
}

llvm::Value *llvmNewLocalStringVar(const char* data, size_t len, llvm::Module *module)
{
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

#pragma mark - Objc factories

llvm::Value *ObjCCodeGen::newString(std::string string){
    const char *name = string.c_str();
   
    llvm::Value *strGlobal = llvmNewLocalStringVar(name, string.length(), _module);
    llvm::Value *aClass = _builder->CreateCall(_module->getFunction("objc_getClass"),  llvmNewLocalStringVar(std::string("NSString"), _module), "calltmp");
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string("stringWithUTF8String:"), _module), "calltmp");
    
    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(aClass);
    ArgsV.push_back(sel);
    ArgsV.push_back(strGlobal);

    llvm::Value *value = _builder->CreateCall(_module->getFunction("objc_msgSend"), ArgsV, "objc_msgSend");
    return value;
}

llvm::Value *ObjCCodeGen::newNumber(double doubleValue){
    llvm::Value *aClass = _builder->CreateCall(_module->getFunction("objc_getClass"),  llvmNewLocalStringVar(std::string("NSNumber"), _module), "calltmp");
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string("numberWithDouble:"), _module), "calltmp");
    auto constValue = llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(doubleValue));

    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(aClass);
    ArgsV.push_back(sel);
    ArgsV.push_back(constValue);

    llvm::Value *value = _builder->CreateCall(_module->getFunction("objc_msgSend"), ArgsV, "objc_msgSend");
    return value;
}

llvm::Value *ObjCCodeGen::newNumberWithLLVMValue(llvm::Value *doubleValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string("numberWithDouble:"), _module), "calltmp");
    llvm::Value *aClass = _builder->CreateCall(_module->getFunction("objc_getClass"),  llvmNewLocalStringVar(std::string("NSNumber"), _module), "calltmp");
    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(aClass);
    ArgsV.push_back(sel);
    ArgsV.push_back(doubleValue);
    
    llvm::Value *value = _builder->CreateCall(_module->getFunction("objc_msgSend"), ArgsV, "objc_msgSend");
    return value;
}

llvm::Value *ObjCCodeGen::doubleValue(llvm::Value *llvmValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), llvmNewLocalStringVar(std::string("doubleValue"), _module), "calltmp");

    std::vector<llvm::Value*> ArgsV;
    ArgsV.push_back(llvmValue);
    ArgsV.push_back(sel);

    llvm::Value *value = _builder->CreateCall(_module->getFunction("objc_msgSend_fpret"), ArgsV, "objc_msgSend_fpret");
    return value;
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
//  Print(" RegExp(");
//  PrintLiteral(node->pattern(), false);
//  Print(",");
//  PrintLiteral(node->flags(), false);
//  Print(") ");
}


void ObjCCodeGen::VisitObjectLiteral(ObjectLiteral* node) {
//  Print("{ ");
//  for (int i = 0; i < node->properties()->length(); i++) {
//    if (i != 0) Print(",");
//    ObjectLiteral::Property* property = node->properties()->at(i);
//    Print(" ");
//    Visit(property->key());
//    Print(": ");
//    Visit(property->value());
//  }
//  Print(" }");
}


void ObjCCodeGen::VisitArrayLiteral(ArrayLiteral* node) {
//  Print("[ ");
//  for (int i = 0; i < node->values()->length(); i++) {
//    if (i != 0) Print(",");
//    Visit(node->values()->at(i));
//  }
//  Print(" ]");
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
    
    VisitStartAccumulation(node->value());
    
    // Store the value.
    switch (assign_type) {
        case VARIABLE: {
            VariableProxy *target = (VariableProxy *)node->target();
            assert(target);
            std::string str = stringFromV8AstRawString(target->raw_name());
            //TODO : respect scopes!
            llvm::AllocaInst *alloca;
            if (!_namedValues[str]){
                alloca = _builder->CreateAlloca(ObjcPointerTy(), 0, str);
                _namedValues[str] = alloca;
            } else {
                alloca = _namedValues[str];
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
    std::string str = stringFromV8AstRawString(node->raw_name());
    auto varValue = _namedValues[str];
    PushValueToContext(_builder->CreateLoad(varValue, str));
}

void ObjCCodeGen::VisitYield(Yield* node) {
//  Print("yield ");
//  Visit(node->expression());
}


void ObjCCodeGen::VisitThrow(Throw* node) {
//  Print("throw ");
//  Visit(node->exception());
}


void ObjCCodeGen::VisitProperty(Property* node) {
//  Expression* key = node->key();
//  Literal* literal = key->AsLiteral();
//  if (literal != NULL && literal->value()->IsInternalizedString()) {
//    Print("(");
//    Visit(node->obj());
//    Print(").");
//    PrintLiteral(literal->value(), false);
//  } else {
//    Visit(node->obj());
//    Print("[");
//    Visit(key);
//    Print("]");
//  }
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
        VisitStartAccumulation(args->at(i));
    }
   
    if (_accumulatorContext->back() == 0) {
        return;
    }
   
    if (!calleeF->isVarArg()) {
        assert(calleeF->arg_size() == args->length() && "Unknown function referenced: airity mismatch");
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
        finalArgs.push_back(arg);
    }
   
    if (_context) {
        std::cout << '\n' << __PRETTY_FUNCTION__ << "Context size:" << _context->size();
    }
    
    std::reverse(finalArgs.begin(), finalArgs.end());
    //TODO : a function call that is not assigned will push
    //an unnecessary value to the stack!
    PushValueToContext(_builder->CreateCall(calleeF, finalArgs, "calltmp"));
}


void ObjCCodeGen::VisitCallNew(CallNew* node) {
//  Print("new (");
//  Visit(node->expression());
//  Print(")");
//  PrintArguments(node->arguments());
}


void ObjCCodeGen::VisitCallRuntime(CallRuntime* node) {
//  Print("%%");
//  PrintLiteral(node->name(), false);
//  PrintArguments(node->arguments());
}


void ObjCCodeGen::VisitUnaryOperation(UnaryOperation* node) {
//  Token::Value op = node->op();
//  bool needsSpace =
//      op == Token::DELETE || op == Token::TYPEOF || op == Token::VOID;
//  Print("(%s%s", Token::String(op), needsSpace ? " " : "");
//  Visit(node->expression());
//  Print(")");
}


void ObjCCodeGen::VisitCountOperation(CountOperation* node) {

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
    
}

void ObjCCodeGen::VisitLogicalExpression(BinaryOperation* expr) {
    
}

void ObjCCodeGen::VisitArithmeticExpression(BinaryOperation* expr) {
    Expression *left = expr->left();
    Expression *right = expr->right();
   
    VisitStartStackAccumulation(left);
    auto lhs = PopContext();
    
    VisitStartAccumulation(right);
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
//        case '<':
//            L = Builder.CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
//            return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()),
//                                        "booltmp");
        default: break;
    }
    
    PushValueToContext(result);
}
    
 
void ObjCCodeGen::VisitCompareOperation(CompareOperation* node) {
//  Print("(");
//  Visit(node->left());
//  Print(" %s ", Token::String(node->op()));
//  Visit(node->right());
//  Print(")");
}


void ObjCCodeGen::VisitThisFunction(ThisFunction* node) {
//  Print("<this-function>");
}


void ObjCCodeGen::VisitStartAccumulation(AstNode *expr) {
    if (_context != NULL && _context == _accumulatorContext) {
        Visit(expr);
        return;
    }
    
    if (!_accumulatorContext) {
        _accumulatorContext = new std::vector<llvm::Value *>;
    }
    _context = _accumulatorContext;
    Visit(expr);
}

void ObjCCodeGen::EndAccumulation() {
    assert(0);
    delete _accumulatorContext;
    _accumulatorContext = NULL;
}

void ObjCCodeGen::VisitStartStackAccumulation(AstNode *expr) {
    if (!_stackAccumulatorContext) {
        _stackAccumulatorContext = new std::vector<llvm::Value *>;
    }
    _context = _stackAccumulatorContext;
    Visit(expr);
}

void ObjCCodeGen::EndStackAccumulation(){
    assert(0);
    delete _stackAccumulatorContext;
    _stackAccumulatorContext = NULL;
}

void ObjCCodeGen::PushValueToContext(llvm::Value *value) {
    if (!_context) {
        _context = new std::vector<llvm::Value *>;
        return;
    }
    if (value) {
        _context->push_back(value);
    } else {
        printf("warning: tried to push null value");
    }
}

llvm::Value *ObjCCodeGen::PopContext() {
    if (!_context || !_context->size()) {
        return NULL;
    }
    llvm::Value *value = _context->back();
    _context->pop_back();
    return value;
}
