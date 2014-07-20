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
#include "cgobjcjsruntime.h"

#define DEBUG 1
#define ILOG(A, ...) if (DEBUG) printf(A,##__VA_ARGS__);

using namespace v8::internal;

static std::string stringFromV8AstRawString(const AstRawString *raw) {
    std::string str;
    size_t size = raw->length();
    const unsigned char *data = raw->raw_data();
    for (int i = 0; i < size; i++) {
        str += data[i];
    }
    return str;
}

static std::string FUNCTION_CMD_ARG_NAME("__cmd__");
static std::string FUNCTION_THIS_ARG_NAME("__this");

static std::string DEFAULT_RET_ALLOCA_NAME("DEFAULT_RET_ALLOCA");
static std::string SET_RET_ALLOCA_NAME("SET_RET_ALLOCA");

CGObjCJS::CGObjCJS(Zone *zone){
    InitializeAstVisitor(zone);
    llvm::LLVMContext &Context = llvm::getGlobalContext();
    _builder = new llvm::IRBuilder<> (Context);
    _module = new llvm::Module("jit", Context);
    //TODO : use llvm to generate this
    _module->setDataLayout("e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128");
    _module->setTargetTriple("x86_64-apple-macosx10.9.0");
    _context = new CGContext();

    _runtime = new CGObjCJSRuntime(_builder, _module);
}

/// CreateArgumentAllocas - Create an alloca for each argument and register the
/// argument in the symbol table so that references to it will succeed.
void CGObjCJS::CreateArgumentAllocas(llvm::Function *F, v8::internal::Scope* scope) {
    llvm::Function::arg_iterator AI = F->arg_begin();
    int numParams = scope->num_parameters();
    for (unsigned Idx = 0, e = numParams; Idx != e; ++Idx, ++AI) {
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
void CGObjCJS::CreateJSArgumentAllocas(llvm::Function *F, v8::internal::Scope* scope) {
    llvm::Function::arg_iterator AI = F->arg_begin();
    llvm::Value *argSelf = AI++;
    int numParams = scope->num_parameters();
   
    localStringVar(FUNCTION_THIS_ARG_NAME, _module);
    llvm::AllocaInst *allocaThis = CreateEntryBlockAlloca(F, FUNCTION_THIS_ARG_NAME);
    _builder->CreateStore(argSelf, allocaThis);
    _context->setValue(FUNCTION_THIS_ARG_NAME, allocaThis);
   
    localStringVar(FUNCTION_CMD_ARG_NAME, _module);
    llvm::AllocaInst *allocaCMD = CreateEntryBlockAlloca(F, FUNCTION_CMD_ARG_NAME);
    llvm::Value *argCMD = AI++;
    _builder->CreateStore(argCMD, allocaCMD);
    _context->setValue(FUNCTION_CMD_ARG_NAME, allocaCMD);
    
    for (unsigned Idx = 0, e = numParams; Idx != e; ++Idx, ++AI) {
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

void CGObjCJS::dump(){
    _module->dump();
}

void CGObjCJS::VisitVariableDeclaration(v8::internal::VariableDeclaration* node) {
    //TODO : Enter into symbol table with scope..
    std::string str = stringFromV8AstRawString(node->proxy()->raw_name());
    llvm::Function *f = _builder->GetInsertBlock()->getParent();
    llvm::AllocaInst *alloca = CreateEntryBlockAlloca(f, str);
    _context->setValue(str, alloca);

    VariableProxy *var = node->proxy();
    //TODO : insert points
    Visit(var);
}

void CGObjCJS::VisitFunctionDeclaration(v8::internal::FunctionDeclaration* node) {
    std::string name = stringFromV8AstRawString(node->fun()->raw_name());
    if (!name.length()){
        ILOG("TODO: support unnamed functions");
    }

    bool isJSFunction = name != std::string("objcjs_main") && name != std::string("NSLog");
    if (isJSFunction) {
        assert(!_module->getFunction(name) && "function is already declared");
    }
    
    int numParams = node->fun()->scope()->num_parameters();
    auto function = CGObjCJSFunction(numParams, name, _module);
    
    if (isJSFunction){
        //Add define JSFunction to front of main
        llvm::Function *main = _module->getFunction("main");
        auto nameAlloca = localStringVar(name.c_str(), name.length(), _module);
        std::vector<llvm::Value*> ArgsV;
        ArgsV.push_back(nameAlloca);
        ArgsV.push_back(function);
        auto call = llvm::CallInst::Create(_module->getFunction("defineJSFunction"), ArgsV, "calltmp");
        llvm::BasicBlock *mainBB = &main->getBasicBlockList().front();
        mainBB->getInstList().push_front(call);

        //If this is a nested declaration set the parent to this!
        if (_builder->GetInsertBlock() && _builder->GetInsertBlock()->getParent()) {
            auto functionClass = _runtime->classNamed(name.c_str());
            auto jsThis = _builder->CreateLoad(_context->valueForKey(FUNCTION_THIS_ARG_NAME) , "load-this");
            _runtime->messageSend(functionClass, "setParent:", jsThis);
        }
    }
    
    auto functionAlloca = _builder->CreateAlloca(ObjcPointerTy());
    _builder->CreateStore(_runtime->classNamed(name.c_str()), functionAlloca);
    _context->setValue(name, functionAlloca);
    
    //VisitFunctionLiteral
    VisitStartAccumulation(node->fun(), false);
}

#pragma mark - Modules

void CGObjCJS::VisitModuleDeclaration(ModuleDeclaration* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitImportDeclaration(ImportDeclaration* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitExportDeclaration(ExportDeclaration* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitModuleLiteral(ModuleLiteral* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitModuleVariable(ModuleVariable* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitModulePath(ModulePath* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitModuleUrl(ModuleUrl* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitModuleStatement(ModuleStatement* node) {
    UNIMPLEMENTED();
}

#pragma mark - Statements

void CGObjCJS::VisitExpressionStatement(ExpressionStatement* node) {
    Visit(node->expression());
}

void CGObjCJS::VisitEmptyStatement(EmptyStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitIfStatement(IfStatement* node) {
    CGIfStatement(node, true);
}

void CGObjCJS::CGIfStatement(IfStatement *node, bool flag){
    Visit(node->condition());
    llvm::Value *condV = PopContext();
    if (!condV) {
        return;
    }

    // Convert condition to a bool by comparing equal to NullPointer.
    // in JS land bools are represented as ints
    // so comparing a NSDoubleNumber that is 0.0 will be incorrect in JS
    // so get the int value
    condV = _builder->CreateICmpEQ(_runtime->boolValue(condV), ObjcNullPointer(), "ifcond");
    
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
    }
    
    if (!thenV){
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
    }
    
    if (!elseV) {
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

void CGObjCJS::VisitContinueStatement(ContinueStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitBreakStatement(BreakStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitReturnStatement(ReturnStatement* node) {
    Visit(node->expression());
    llvm::BasicBlock *block = _builder->GetInsertBlock();
   
    auto blockName = block->getName();
    if (blockName.startswith(llvm::StringRef("then")) ||
        blockName.startswith(llvm::StringRef("else"))
        ){
        //In an if statement, the PHINode needs to have
        //the type 0 assigned to it!
        llvm::AllocaInst *alloca = _context->valueForKey(SET_RET_ALLOCA_NAME);
        _builder->CreateStore(PopContext(), alloca);
        //TODO : remove this!
        PushValueToContext(ObjcNullPointer());
        return;
    }

    llvm::Function *currentFunction = block->getParent();
    assert(currentFunction->getReturnType() == ObjcPointerTy() && _context->size());
    llvm::AllocaInst *alloca = _context->valueForKey(DEFAULT_RET_ALLOCA_NAME);
    auto retValue = PopContext();
    assert(retValue && "requires return value" && alloca && "requires return alloca");
    _builder->CreateStore(retValue, alloca);
}

void CGObjCJS::VisitWithStatement(WithStatement* node) {
    UNIMPLEMENTED(); //Deprecated
}

#pragma mark - Switch

void CGObjCJS::VisitSwitchStatement(SwitchStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitCaseClause(CaseClause* clause) {
    UNIMPLEMENTED();
}

#pragma mark - Loops

void CGObjCJS::VisitDoWhileStatement(DoWhileStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitWhileStatement(WhileStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitForStatement(ForStatement* node) {
    _context->EmptyStack();
   
    //TODO : handle shadow variables
    if (node->init()){
        Visit(node->init());
        PopContext();
    }
    
    auto function = _builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(_module->getContext(), "forloop", function);
    
    //break to loop from current position
    _builder->CreateBr(loopBB);
    _builder->SetInsertPoint(loopBB);
    
    Visit(node->body());
    _context->EmptyStack();
    
    Visit(node->cond());
    auto condVar = PopContext();

    Visit(node->next());
    _context->EmptyStack();

    auto endCond = _builder->CreateICmpEQ(_runtime->boolValue(condVar), ObjcNullPointer(), "loopcond");

    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(_module->getContext(), "afterloop", function);
    _builder->CreateCondBr(endCond, afterBB, loopBB);
   
   
    _builder->SetInsertPoint(afterBB);
}

void CGObjCJS::VisitForInStatement(ForInStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitForOfStatement(ForOfStatement* node) {
    UNIMPLEMENTED();
}

#pragma mark - Try
void CGObjCJS::VisitTryCatchStatement(TryCatchStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitTryFinallyStatement(TryFinallyStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitDebuggerStatement(DebuggerStatement* node) {
    UNIMPLEMENTED();
}

//Define the body of the function
void CGObjCJS::VisitFunctionLiteral(v8::internal::FunctionLiteral* node) {
    enterContext();
    _builder->saveIP();
    
    auto name = stringFromV8AstRawString(node->raw_name());
    
    if (!name.length()){
        ILOG("TODO: support unnamed functions");
        VisitDeclarations(node->scope()->declarations());
        VisitStatements(node->body());
        
        return;
    }
    
    auto function = _module->getFunction(name);

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", function);
    _builder->SetInsertPoint(BB);
   
    bool isJSFunction = !(name == std::string("objcjs_main") || name == std::string("NSLog"));
    if (isJSFunction) {
        CreateJSArgumentAllocas(function, node->scope());
    } else {
        CreateArgumentAllocas(function, node->scope());
    }
   
    //Return types
//TODO : don't malloc a return sentenenial everytime and use builtin malloc func
    auto sentenialReturnAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, std::string("sentential"));
    llvm::ConstantInt* const_int64_10 = llvm::ConstantInt::get(_module->getContext(), llvm::APInt(64, llvm::StringRef("8"), 10));
    auto retPtr = _builder->CreateCall(_module->getFunction("malloc"), const_int64_10, "calltmp");
    _builder->CreateStore(retPtr, sentenialReturnAlloca);
    auto sentenialReturnValue = _builder->CreateLoad(sentenialReturnAlloca);
    
    //end ret alloca is the return value at the end of a function
    auto endRetAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, std::string("endret"));
    _builder->CreateStore(ObjcNullPointer(), endRetAlloca);
    //TODO : instead of returnig a null pointer, return a 'undefined' sentenial
    _context->setValue(DEFAULT_RET_ALLOCA_NAME , endRetAlloca);
    
    auto retAlloca =  _builder->CreateAlloca(ObjcPointerTy(), 0, std::string("ret"));
    _builder->CreateStore(sentenialReturnValue, retAlloca);
    _context->setValue(SET_RET_ALLOCA_NAME, retAlloca);

    auto value = _context->valueForKey(SET_RET_ALLOCA_NAME);
    assert(value);
    _builder->saveIP();
    
    auto insertBlock = _builder->GetInsertBlock();
    VisitDeclarations(node->scope()->declarations());
    _builder->SetInsertPoint(insertBlock);
  
    VisitStatements(node->body());
    
    assert(function->getReturnType() == ObjcPointerTy() && "all functions return pointers");
    
    auto retValue = _builder->CreateLoad(retAlloca, "retalloca");
   
    //If the return value was set, then return what it was set to
    auto condV = _builder->CreateICmpEQ(retValue, sentenialReturnValue, "ifsetreturn");
   
    auto setRetBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "setret", function);
    auto defaultRetBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "endret");
    _builder->CreateCondBr(condV, defaultRetBB, setRetBB);
    
    _builder->saveIP();
    
    function->getBasicBlockList().push_back(defaultRetBB);
    _builder->SetInsertPoint(defaultRetBB);
    auto endRetValue = _builder->CreateLoad(_context->valueForKey(DEFAULT_RET_ALLOCA_NAME), "endretalloca");
    _builder->CreateRet(endRetValue);
    _builder->saveAndClearIP();

    _builder->SetInsertPoint(setRetBB);
    _builder->CreateRet(_builder->CreateLoad(retAlloca, "retallocaend"));
    _builder->saveAndClearIP();
    
    if (_context) {
        std::cout << "Context size:" << _context->size();
    }
    
    EndAccumulation();
}

void CGObjCJS::VisitNativeFunctionLiteral(NativeFunctionLiteral* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitConditional(Conditional* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitLiteral(class Literal* node) {
    CGLiteral(node->value(), true);
}

#pragma mark - Literals

llvm::Value *CGObjCJS::CGLiteral(Handle<Object> value, bool push) {
    Object* object = *value;
    llvm::Value *lvalue = NULL;
    if (object->IsString()) {
        String* string = String::cast(object);
        auto name = asciiStringWithV8String(string);
        if (name.length()){
            lvalue = _runtime->newString(name);
        }
        
        ILOG("STRING literal: %s", name.c_str());
    } else if (object->IsNull()) {
        lvalue = _runtime->newNumber(0);
        ILOG("null literal");
        lvalue = ObjcNullPointer();
    } else if (object->IsTrue()) {
        ILOG("true literal");
        lvalue = _runtime->newNumber(1);
    } else if (object->IsFalse()) {
        ILOG("false literal");
        lvalue = _runtime->newNumber(0);
    } else if (object->IsUndefined()) {
        ILOG("undefined");
    } else if (object->IsNumber()) {
        lvalue = _runtime->newNumber(object->Number());
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

void CGObjCJS::VisitRegExpLiteral(RegExpLiteral* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitObjectLiteral(ObjectLiteral* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitArrayLiteral(ArrayLiteral* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitVariableProxy(VariableProxy* node) {
    EmitVariableLoad(node);
}

//TODO : checkout full-codegen-x86.cc
void CGObjCJS::VisitAssignment(Assignment* node) {
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
   
    //TODO : new scope
    Visit(node->value());
    
    // Store the value.
    switch (assign_type) {
        case VARIABLE: {
            VariableProxy *targetProxy = (VariableProxy *)node->target();
            assert(targetProxy && "target for assignment required");
            std::string targetName = stringFromV8AstRawString(targetProxy->raw_name());
            
            llvm::Value *value = PopContext();
            assert(value && "missing value - not implemented");
            
            if (_context->valueForKey(targetName)){
                llvm::AllocaInst *targetAlloca = _context->valueForKey(targetName);
                _builder->CreateStore(value, targetAlloca);
                
                //Store all assignements in environment if present
                auto jsThisAlloca = _context->valueForKey(FUNCTION_THIS_ARG_NAME);
                
                if (jsThisAlloca) {
                    auto jsThis = _builder->CreateLoad(jsThisAlloca);

                    auto keypathStringValueAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, "alloca");
                    _builder->CreateStore(_runtime->newString(targetName), keypathStringValueAlloca);
                    auto keypathStringValue = _builder->CreateLoad(keypathStringValueAlloca);
                   
                    //Declare the key in this environment
                    std::vector<llvm::Value *>Args;
                    Args.push_back(value);
                    Args.push_back(keypathStringValue);
                    _runtime->messageSend(jsThis, "_objcjs_env_setValue:declareKey:", Args);
                }
            } else {
                //Set the environment value
                auto jsThisAlloca = _context->valueForKey(FUNCTION_THIS_ARG_NAME);
                auto jsThis = _builder->CreateLoad(jsThisAlloca);
                
                auto keypathStringValueAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, "alloca");
                _builder->CreateStore(_runtime->newString(targetName), keypathStringValueAlloca);
                auto keypathStringValue = _builder->CreateLoad(keypathStringValueAlloca);
               
                std::vector<llvm::Value *>Args;
                Args.push_back(value);
                Args.push_back(keypathStringValue);
                _runtime->messageSend(jsThis, "_objcjs_env_setValue:forKey:", Args);
            }
            
            //assignment returns 0
//            PushValueToContext(ObjcNullPointer());
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
void CGObjCJS::EmitBinaryOp(BinaryOperation* expr, Token::Value op){
//TODO:
}

void CGObjCJS::EmitVariableAssignment(Variable* var, Token::Value op) {
//TODO:
}

void CGObjCJS::EmitVariableLoad(VariableProxy* node) {
    std::string variableName = stringFromV8AstRawString(node->raw_name());
    auto varAlloca = _context->valueForKey(variableName);
    if (varAlloca) {
        PushValueToContext(_builder->CreateLoad(varAlloca, variableName));
        return;
    }
   
    //NTS: always create an alloca for an arugment you want to pass to a function!!
    auto environmentVariableAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, "env_var_alloca");
    _builder->CreateStore(_runtime->newString(variableName), environmentVariableAlloca);
    auto keypathArgument = _builder->CreateLoad(environmentVariableAlloca);
   
    //load the value from the parents environment
    //TODO : support N layers of nested functions
    auto parentFunction = _builder->GetInsertBlock()->getParent();
    auto parentName = parentFunction ->getName().str();
    auto parentThis = _runtime->messageSend(_runtime->classNamed(parentName.c_str()), "parent");
    auto value = _runtime->messageSend(parentThis, "_objcjs_env_valueForKey:", keypathArgument);

    PushValueToContext(value);
}

void CGObjCJS::VisitYield(Yield* node) {
    UNIMPLEMENTED(); //Deprecated
}

void CGObjCJS::VisitThrow(Throw* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitProperty(Property* node) {
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

void CGObjCJS::VisitCall(Call* node) {
    Expression *callee = node->expression();
    Call::CallType callType = GetCallType(node, isolate());
   
    std::string name;
    
    VariableProxy* proxy = callee->AsVariableProxy();
    if (callType == Call::GLOBAL_CALL) {
        UNIMPLEMENTED();
    } else if (callType == Call::LOOKUP_SLOT_CALL) {
        // Call to a lookup slot (dynamically introduced variable).
        UNIMPLEMENTED();
    } else if (callType == Call::OTHER_CALL){
        VariableProxy* proxy = callee->AsVariableProxy();
        name = stringFromV8AstRawString(proxy->raw_name());
    } else {
        UNIMPLEMENTED();
    }

    ZoneList<Expression*>* args = node->arguments();
    for (int i = 0; i <args->length(); i++) {
        //TODO : this should likely retain the values
        Visit(args->at(i));
    }
    
    std::vector<llvm::Value *> finalArgs;
    for (unsigned i = 0; i < args->length(); i++){
        llvm::Value *arg = PopContext();
        finalArgs.push_back(arg);
    }
    
    std::reverse(finalArgs.begin(), finalArgs.end());

    if (_context) {
        std::cout << '\n' << __PRETTY_FUNCTION__ << "Context size:" << _context->size();
    }

    bool isJSFunction = !(name == std::string("objcjs_main") || name == std::string("NSLog"));
    if (isJSFunction) {
        //Create a new instance and invoke the body
       
        llvm::Value *target;
        auto calleeF = _module->getFunction(name);
        if (calleeF) {
            target = _runtime->classNamed(name.c_str());
        } else {
            Visit(callee);
            target = PopContext();
       }
        
        auto value = _runtime->messageSendJSFunction(target, finalArgs);
        PushValueToContext(value);
    } else {
        auto calleeF = _module->getFunction(name);
        PushValueToContext(_builder->CreateCall(calleeF, finalArgs, "calltmp"));
    }
}

void CGObjCJS::VisitCallNew(CallNew* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitCallRuntime(CallRuntime* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitUnaryOperation(UnaryOperation* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitCountOperation(CountOperation* node) {
    enum LhsKind { VARIABLE, NAMED_PROPERTY, KEYED_PROPERTY };
    LhsKind assign_type = VARIABLE;
    Property* prop = node->expression()->AsProperty();
    // In case of a property we use the uninitialized expression context
    // of the key to detect a named property.
    if (prop != NULL) {
        assign_type =
        (prop->key()->IsPropertyName()) ? NAMED_PROPERTY : KEYED_PROPERTY;
    }
   
    if (assign_type == VARIABLE){
        EmitVariableLoad(node->expression()->AsVariableProxy());
    }
    
    if (node->is_postfix()) {
        
        
    }
    if (node->is_prefix()) {
        UNIMPLEMENTED();
    }
}

void CGObjCJS::VisitBinaryOperation(BinaryOperation* expr) {
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

void CGObjCJS::VisitComma(BinaryOperation* expr) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitLogicalExpression(BinaryOperation* expr) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitArithmeticExpression(BinaryOperation* expr) {
    Expression *left = expr->left();
    Expression *right = expr->right();
  
    //TODO : stack scope
    Visit(left);
    auto lhs = PopContext();
    
    //TODO : stack scope
    Visit(right);
    auto rhs = PopContext();
   
    llvm::Value *result = NULL;
    std::vector<llvm::Value *>Args;
    Args.push_back(lhs);
    Args.push_back(rhs);

    auto op = expr->op();
    switch (op) {
        case v8::internal::Token::ADD : {
            result = _runtime->messageSend(lhs, "objcjs_add:", rhs);
            break;
        }
        case v8::internal::Token::SUB : {
            result = _runtime->messageSend(lhs, "objcjs_subtract:", rhs);
            break;
        }
        case v8::internal::Token::MUL : {
            result = _runtime->messageSend(lhs, "objcjs_multiply:", rhs);
            break;
        }
        case v8::internal::Token::DIV : {
            result = _runtime->messageSend(lhs, "objcjs_divide:", rhs);
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
 
void CGObjCJS::VisitCompareOperation(CompareOperation* expr) {
      ASSERT(!HasStackOverflow());
    llvm::Value *resultValue = NULL;
    
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
        resultValue = _runtime->messageSend(left, "isLessThan:",  right);
    } else if (op == Token::GT){
        resultValue = _runtime->messageSend(left, "isGreaterThan:",  right);
    } else if (op == Token::LTE){
        resultValue = _runtime->messageSend(left, "isLessThanOrEqualTo:",  right);
    } else if (op == Token::GTE){
        resultValue = _runtime->messageSend(left, "isGreaterThanOrEqualTo:",  right);
    } else if (op == Token::EQ){
//TODO : implement equality with respect to ECMAScript conventions
        resultValue = _runtime->messageSend(left, "isEqual:", right);
    } else if (op == Token::EQ_STRICT){
        resultValue = _runtime->messageSend(left, "isEqual:", right);
    }
    
    assert(resultValue);
    PushValueToContext(resultValue);
}

void CGObjCJS::VisitThisFunction(ThisFunction* node) {
    UNIMPLEMENTED();
}

void CGObjCJS::VisitStartAccumulation(AstNode *expr, bool extendContext) {
    if (extendContext){
        enterContext();
    }
    
    Visit(expr);
}

void CGObjCJS::enterContext(){
    CGContext *context;
    if (_context) {
        context = _context->Extend();
    } else {
        context = new CGContext();
    }
    
    Contexts.push_back(context);
    _context = context;   
}

void CGObjCJS::VisitStartAccumulation(AstNode *expr) {
    VisitStartAccumulation(expr, false);
}

void CGObjCJS::EndAccumulation() {
    Contexts.pop_back();
    delete _context;
    auto context = Contexts.back();
    _context = context;
}

void CGObjCJS::VisitStartStackAccumulation(AstNode *expr) {
    //TODO : retain values
    VisitStartAccumulation(expr, false);
}

void CGObjCJS::EndStackAccumulation(){
    assert(0);
    delete _context;
    auto context = Contexts.back();
    Contexts.pop_back();
    _context = context;
}

void CGObjCJS::PushValueToContext(llvm::Value *value) {
    _context->Push(value);
}

llvm::Value *CGObjCJS::PopContext() {
    return _context->Pop();
}
