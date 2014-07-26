//
//  codegen.cpp
//  objcjs
//
//  Created by Jerry Marino on 7/6/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include "cgobjcjs.h"
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
#define ILOG(A, ...) if (DEBUG){ printf(A,##__VA_ARGS__), printf("\n");}

using namespace v8::internal;

#define CALL_LOG(STR)\
    _builder->CreateCall(_module->getFunction("NSLog"), _runtime->newString(STR))

static std::string stringFromV8AstRawString(const AstRawString *raw) {
    std::string str;
    size_t size = raw->length();
    const unsigned char *data = raw->raw_data();
    for (int i = 0; i < size; i++) {
        str += data[i];
    }
    return str;
}

static std::string FUNCTION_CMD_ARG_NAME("_cmd");
static std::string FUNCTION_THIS_ARG_NAME("this");

static std::string DEFAULT_RET_ALLOCA_NAME("DEFAULT_RET_ALLOCA");
static std::string SET_RET_ALLOCA_NAME("SET_RET_ALLOCA");

CGObjCJS::CGObjCJS(Zone *zone){
    InitializeAstVisitor(zone);
    llvm::LLVMContext &Context = llvm::getGlobalContext();
    _builder = new llvm::IRBuilder<> (Context);
    _module = new llvm::Module("jit", Context);
    _context = new CGContext();
    _runtime = new CGObjCJSRuntime(_builder, _module);
    
    assignOpSelectorByToken[Token::ASSIGN_ADD] = std::string("objcjs_add:");
    assignOpSelectorByToken[Token::ASSIGN_SUB] = std::string("objcjs_subtract:");
    assignOpSelectorByToken[Token::ASSIGN_MUL] = std::string("objcjs_multiply:");
    assignOpSelectorByToken[Token::ASSIGN_DIV] = std::string("objcjs_divide:");
    assignOpSelectorByToken[Token::ASSIGN_MOD] = std::string("objcjs_mod:");
    
    assignOpSelectorByToken[Token::ASSIGN_BIT_OR] = std::string("objcjs_bitor:");
    assignOpSelectorByToken[Token::ASSIGN_BIT_XOR] = std::string("objcjs_bitxor:");
    assignOpSelectorByToken[Token::ASSIGN_BIT_AND] = std::string("objcjs_bitand:");
    
    assignOpSelectorByToken[Token::ASSIGN_SHL] = std::string("objcjs_shiftleft:");
    assignOpSelectorByToken[Token::ASSIGN_SAR] = std::string("objcjs_shiftright:");
    assignOpSelectorByToken[Token::ASSIGN_SHR] = std::string("objcjs_shiftrightright:");
    
    opSelectorByToken[Token::ADD] = std::string("objcjs_add:");
    opSelectorByToken[Token::SUB] = std::string("objcjs_subtract:");
    opSelectorByToken[Token::MUL] = std::string("objcjs_multiply:");
    opSelectorByToken[Token::DIV] = std::string("objcjs_divide:");
    opSelectorByToken[Token::MOD] = std::string("objcjs_mod:");
    
    opSelectorByToken[Token::BIT_OR] = std::string("objcjs_bitor:");
    opSelectorByToken[Token::BIT_XOR] = std::string("objcjs_bitxor:");
    opSelectorByToken[Token::BIT_AND] = std::string("objcjs_bitand:");
    
    opSelectorByToken[Token::SHL] = std::string("objcjs_shiftleft:");
    opSelectorByToken[Token::SAR] = std::string("objcjs_shiftright:");
    opSelectorByToken[Token::SHR] = std::string("objcjs_shiftrightright:");
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
        Variable *param = scope->parameter(Idx);
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

void CGObjCJS::VisitDeclarations(ZoneList<Declaration*>* declarations){
    if (declarations->length() > 0) {
        auto preBB = _builder->GetInsertBlock();
        for (int i = 0; i < declarations->length(); i++) {
            auto BB = _builder->GetInsertBlock();
            Visit(declarations->at(i));
            if (BB){
                _builder->SetInsertPoint(BB);
            } else {
                if (preBB)
                    _builder->SetInsertPoint(preBB);
            }
        }
    }
}

void CGObjCJS::VisitStatements(ZoneList<Statement*>* statements){
    auto preBB = _builder->GetInsertBlock();
    for (int i = 0; i < statements->length(); i++) {
        auto BB = _builder->GetInsertBlock();
        Visit(statements->at(i));
        if (BB){
            _builder->SetInsertPoint(BB);
        } else {
            if (preBB)
                _builder->SetInsertPoint(preBB);
        }
    }
}

void CGObjCJS::VisitBlock(Block *block){
    VisitStatements(block->statements());
}

void CGObjCJS::VisitVariableDeclaration(v8::internal::VariableDeclaration* node) {
    std::string name = stringFromV8AstRawString(node->proxy()->raw_name());
    llvm::Function *function = _builder->GetInsertBlock()->getParent();
    llvm::AllocaInst *alloca = CreateEntryBlockAlloca(function, name);
    _context->setValue(name, alloca);

    VariableProxy *var = node->proxy();
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
    if (isJSFunction){
        //Add define JSFunction to front of main
        llvm::Function *main = _module->getFunction("main");

        llvm::BasicBlock *mainBB = &main->getBasicBlockList().front();
        llvm::Instruction *defineFunction = (llvm::Instruction *)_runtime->defineJSFuction(name.c_str(), numParams);
        mainBB->getInstList().push_front(defineFunction);

        //If this is a nested declaration set the parent to this!
        if (_builder->GetInsertBlock() && _builder->GetInsertBlock()->getParent()) {
            auto functionClass = _runtime->classNamed(name.c_str());
            auto jsThisAlloca = _context->valueForKey(FUNCTION_THIS_ARG_NAME);
            auto jsThis = _builder->CreateLoad(jsThisAlloca, "load-this");
            _runtime->messageSend(functionClass, "_objcjs_setParent:", jsThis);
        }
    }
    
    auto functionAlloca = _builder->CreateAlloca(ObjcPointerTy());
    _builder->CreateStore(_runtime->classNamed(name.c_str()), functionAlloca);
    _context->setValue(name, functionAlloca);
    //VisitFunctionLiteral
    Visit(node->fun());
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

    if (node->HasThenStatement()){
        Visit(node->then_statement());
        PopContext();
    }
    
    // Codegen of 'Then' can change the current block
    thenBB = _builder->GetInsertBlock();
    if (!thenBB->getTerminator()) {
        _builder->CreateBr(mergeBB);
    }
    
    // Emit else block.
    function->getBasicBlockList().push_back(elseBB);
    _builder->SetInsertPoint(elseBB);

    if (node->HasElseStatement()) {
        Visit(node->else_statement());
        PopContext();
    }
    
    // Codegen of 'Else' can change the current block
    elseBB = _builder->GetInsertBlock();
    if (!elseBB->getTerminator()) {
        _builder->CreateBr(mergeBB);
    }
    
    // Emit merge block.
    function->getBasicBlockList().push_back(mergeBB);
    _builder->SetInsertPoint(mergeBB);
}

//Continues & breaks must be nested inside of for loops
void CGObjCJS::VisitContinueStatement(ContinueStatement* node) {
    auto currentBlock = _builder->GetInsertBlock();
    auto function = _builder->GetInsertBlock()->getParent();
    bool start = false;
    llvm::BasicBlock *jumpTarget = NULL;
    for (llvm::Function::iterator block = function->end(), e = function->begin(); block != e; --block){
        if (block == *currentBlock) {
            start = true;
        }
        
        if (start && block->getName().startswith(llvm::StringRef("loop.next"))) {
            jumpTarget = block;
            break;
        }
    }
  
    if (!jumpTarget) {
        assert(0 && "invalid continue statement");
    }
    
    _builder->CreateBr(jumpTarget);
}

void CGObjCJS::VisitBreakStatement(BreakStatement* node) {
    auto currentBlock = _builder->GetInsertBlock();
    auto function = _builder->GetInsertBlock()->getParent();
    
    bool start = false;
    llvm::BasicBlock *jumpTarget = NULL;
    for (llvm::Function::iterator block = function->end(), e = function->begin(); block != e; --block){
        if (block == *currentBlock) {
            start = true;
        }
        
        if (start && block->getName().startswith(llvm::StringRef("loop.after"))) {
            jumpTarget = block;
            break;
        }
    }
  
    if (!jumpTarget) {
        assert(0 && "invalid break statement");
    }
    
    _builder->CreateBr(jumpTarget);
}

//Insert the expressions into a returning block
void CGObjCJS::VisitReturnStatement(ReturnStatement* node) {
    llvm::BasicBlock *block = _builder->GetInsertBlock();
    llvm::Function *function = block->getParent();
    auto name = function->getName();
    std::cout << &name;
    
    llvm::BasicBlock *jumpBlock = llvm::BasicBlock::Create(_builder->getContext(), "retjump");
    _builder->SetInsertPoint(block);
    
    _builder->CreateBr(jumpBlock);
    
    function->getBasicBlockList().push_back(jumpBlock);
    _builder->SetInsertPoint(jumpBlock);
   
    Visit(node->expression());
    
    assert(function->getReturnType() == ObjcPointerTy() && "requires pointer return");
    
    llvm::AllocaInst *alloca = _context->valueForKey(SET_RET_ALLOCA_NAME);
    auto retValue = PopContext();
    
    if (retValue) {
        _builder->CreateLoad(alloca);
        _builder->CreateStore(retValue, alloca);
    } else {
        _builder->CreateStore(ObjcNullPointer(), alloca);
    }
   
    llvm::BasicBlock *setBr = returnBlockByFunction[function];
    
    _builder->CreateBr(setBr);
    _builder->SetInsertPoint(block);
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

void CGObjCJS::VisitWhileStatement(WhileStatement* node) {
    auto function = _builder->GetInsertBlock()->getParent();
    auto afterBB = llvm::BasicBlock::Create(_module->getContext(), "loop.after", function);
    auto loopBB = llvm::BasicBlock::Create(_module->getContext(), "loop.next", function);
    auto startBB = llvm::BasicBlock::Create(_module->getContext(), "loop.start", function);

    _builder->CreateBr(startBB);
    _builder->SetInsertPoint(startBB);
    
    Visit(node->cond());
    auto condVar = PopContext();
    _context->EmptyStack();
    
    auto cond = _builder->CreateICmpEQ(condVar, ObjcNullPointer(), "loop.cond");
    _builder->CreateCondBr(cond, afterBB, loopBB);
    _builder->SetInsertPoint(loopBB);
    
    Visit(node->body());
    _context->EmptyStack();
   
    _builder->CreateBr(startBB);
    
    _builder->SetInsertPoint(afterBB);
}

void CGObjCJS::VisitDoWhileStatement(DoWhileStatement* node) {
    auto function = _builder->GetInsertBlock()->getParent();
    auto afterBB = llvm::BasicBlock::Create(_module->getContext(), "loop.after", function);
    auto loopBB = llvm::BasicBlock::Create(_module->getContext(), "loop.next", function);

    _builder->CreateBr(loopBB);
    _builder->SetInsertPoint(loopBB);
    
    Visit(node->body());
    _context->EmptyStack();

    Visit(node->cond());
    auto condVar = PopContext();
    _context->EmptyStack();

    auto endCond = _builder->CreateICmpEQ(condVar, ObjcNullPointer(), "loop.cond");
    _builder->CreateCondBr(endCond, afterBB, loopBB);
   
    _builder->SetInsertPoint(afterBB);
}

void CGObjCJS::VisitForStatement(ForStatement* node) {
    _context->EmptyStack();

    if (node->init()){
        Visit(node->init());
        _context->EmptyStack();
    }

    auto function = _builder->GetInsertBlock()->getParent();
    auto loopBB = llvm::BasicBlock::Create(_module->getContext(), "loop", function);
    auto afterBB = llvm::BasicBlock::Create(_module->getContext(), "loop.after", function);
    auto nextBB = llvm::BasicBlock::Create(_module->getContext(), "loop.next", function);
    auto startBB = llvm::BasicBlock::Create(_module->getContext(), "loop.start", function);
    
    _builder->CreateBr(startBB);
    _builder->SetInsertPoint(startBB);
    
    Visit(node->cond());
    auto condVar = PopContext();
    _context->EmptyStack();
    
    auto continueCond = _builder->CreateICmpEQ((condVar), ObjcNullPointer(), "loop.cond");
    _builder->CreateCondBr(continueCond, afterBB, loopBB);
    _builder->SetInsertPoint(loopBB);

    Visit(node->body());
    Visit(node->next());

    _builder->CreateBr(startBB);

    _builder->SetInsertPoint(nextBB);
    Visit(node->next());
    _builder->CreateBr(loopBB);
    
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

static void CleanupInstructionsAfterBreaks(llvm::Function *function){
    for (llvm::Function::iterator block = function->begin(), e = function->end(); block != e; ++block){
        bool didBreak = false;
        for (llvm::BasicBlock::iterator i = block->begin(), h = block->end();  i && i != h; ++i){
            if (llvm::ReturnInst::classof(i) || llvm::BranchInst::classof(i)){
                didBreak = true;
            } else if (didBreak) {
                while (i && i != h) {
                    i++;
                    if (i) {
                        i->removeFromParent();
                        
                    }
                }
                break;
            }
        }
    }
}

__attribute__((unused))
static void AppendImplicitReturn(llvm::Function *function, llvm::Value *value){
    for (llvm::Function::iterator block = function->begin(), e = function->end(); block != e; ++block){
        if (!block->getTerminator()) {
            //TODO : return undefined
            llvm::ReturnInst::Create(function->getContext(), value, block);
        }
    }
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
        EndAccumulation();
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
   
    //Returns
    auto retAlloca =  _builder->CreateAlloca(ObjcPointerTy(), 0, "__return__");
    _context->setValue(SET_RET_ALLOCA_NAME, retAlloca);
   
    if (isJSFunction) {
        _builder->CreateStore(_builder->CreateLoad(_context->valueForKey(FUNCTION_THIS_ARG_NAME)), retAlloca);
    }
    
    auto value = _context->valueForKey(SET_RET_ALLOCA_NAME);
    assert(value);
    _builder->saveIP();
    
    auto insertBlock = _builder->GetInsertBlock();
    auto setRetBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "return.set");
    returnBlockByFunction[function] = setRetBB;
    
    VisitDeclarations(node->scope()->declarations());
    _builder->SetInsertPoint(insertBlock);
  
    VisitStatements(node->body());
   
    function->getBasicBlockList().push_back(setRetBB);
    
    _builder->SetInsertPoint(setRetBB);
    assert(function->getReturnType() == ObjcPointerTy() && "all functions return pointers");

    _builder->CreateRet(_builder->CreateLoad(retAlloca, "__return__"));
    _builder->saveAndClearIP();
    
    if (_context) {
        ILOG("Context size: %lu", _context->size());
    }
    
    CleanupInstructionsAfterBreaks(function);

    llvm::BasicBlock *front = &function->front();
    auto needsTerminator = !front->getTerminator();
    if (needsTerminator) {
        front->getInstList().push_back(llvm::BranchInst::Create(setRetBB));
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
    
    //TODO : new scope
    Visit(node->value());
    llvm::Value *value = PopContext();
  
    switch (assign_type) {
        case VARIABLE: {
            VariableProxy *targetProxy = (VariableProxy *)node->target();
            assert(targetProxy && "target for assignment required");
            assert(value && "missing value - not implemented");

            std::string targetName = stringFromV8AstRawString(targetProxy->raw_name());
            if (node->op() != Token::EQ && node->op() != Token::INIT_VAR){
                EmitVariableLoad(targetProxy);
                auto target = PopContext();
                auto selector = assignOpSelectorByToken[node->op()].c_str();
                assert(selector && "unsupported assignment operation");
                value = _runtime->messageSend(target, selector, value);
            }
            
            if (node->op() == Token::INIT_VAR) {
                //TODO : refactor
                ILOG("INIT_VAR %s", targetName.c_str());
            }
            
            EmitVariableStore(targetProxy, value);
            break;
        } case NAMED_PROPERTY: {
            Property *property = node->target()->AsProperty();
            EmitProperty(property, value);
            
            break;
        }
        case KEYED_PROPERTY: {
            UNIMPLEMENTED();
            break;
        }
    }
}

void CGObjCJS::EmitProperty(Property *property, llvm::Value *value){
    VariableProxy *object = property->obj()->AsVariableProxy();
    v8::internal::Literal *key = property->key()->AsLiteral();
    auto BB = _builder->GetInsertBlock();

    Visit(key);
    llvm::Value *keyValue = PopContext();
    assert(keyValue);
    
    _builder->SetInsertPoint(BB);
    Visit(object);
    llvm::Value *objValue = PopContext();
    assert(objValue);
    
    _builder->SetInsertPoint(BB);
    std::string objectName = stringFromV8AstRawString(object->AsVariableProxy()->raw_name());
    assert(objectName.length());
    std::string keyName = stringFromV8AstRawString(key->AsRawPropertyName());

    _runtime->assignProperty(objValue, keyName, value);
}

void CGObjCJS::EmitVariableStore(VariableProxy* targetProxy, llvm::Value *value) {
    assert(targetProxy && "target for assignment required");
    assert(value && "missing value - not implemented");
    std::string targetName = stringFromV8AstRawString(targetProxy->raw_name());
    
    llvm::AllocaInst *targetAlloca;
    bool scopeHasTarget = _context->valueForKey(targetName);
    if (scopeHasTarget){
        targetAlloca = _context->valueForKey(targetName);
        _builder->CreateStore(value, targetAlloca);
       
        //TODO : clean this up as it is killing performance
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
        targetAlloca = _builder->CreateAlloca(ObjcPointerTy());
        //Set the environment value
        auto jsThisAlloca = _context->valueForKey(FUNCTION_THIS_ARG_NAME);
        auto jsThis = _builder->CreateLoad(jsThisAlloca);
        
        _builder->CreateStore(_runtime->newString(targetName), targetAlloca);
        auto keypathStringValue = _builder->CreateLoad(targetAlloca);
        
        std::vector<llvm::Value *>Args;
        Args.push_back(value);
        Args.push_back(keypathStringValue);
        _runtime->messageSend(jsThis, "_objcjs_env_setValue:forKey:", Args);
    }
}

void CGObjCJS::EmitVariableLoad(VariableProxy* node) {
    std::string variableName = stringFromV8AstRawString(node->raw_name());
    auto varAlloca = _context->valueForKey(variableName);
    if (varAlloca) {
        PushValueToContext(_builder->CreateLoad(varAlloca, variableName));
        return;
    }
    
    if (symbolIsClass(variableName)){
        PushValueToContext(_runtime->classNamed(variableName.c_str()));
        return;
    }
   
    //NTS: always create an alloca for an arugment you want to pass to a function!!
    auto environmentVariableAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, "env_var_alloca");
    _builder->CreateStore(_runtime->newString(variableName), environmentVariableAlloca);
    auto keypathArgument = _builder->CreateLoad(environmentVariableAlloca);
   
    //load the value from the parents environment
    auto parentFunction = _builder->GetInsertBlock()->getParent();
    auto parentName = parentFunction ->getName().str();
    auto parentThis = _runtime->messageSend(_runtime->classNamed(parentName.c_str()), "_objcjs_parent");
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
    Property *property = node;
    
    VariableProxy *object = property->obj()->AsVariableProxy();
    v8::internal::Literal *key = property->key()->AsLiteral();
    
    Visit(key);
    llvm::Value *keyValue = PopContext();
    assert(keyValue);
    
    Visit(object);
    llvm::Value *objValue = PopContext();
    assert(objValue);
    
    std::string objectName = stringFromV8AstRawString(object->AsVariableProxy()->raw_name());
    std::string keyName = stringFromV8AstRawString(key->AsRawPropertyName());

    _runtime->declareProperty(objValue, keyName);

    PushValueToContext(_runtime->messageSend(objValue, keyName.c_str()));
}

static Call::CallType GetCallType(Expression *call, Isolate* isolate) {
    VariableProxy* proxy = call->AsVariableProxy();
    if (proxy != NULL && proxy->var()) {
        
        if (proxy->var()->IsUnallocated()) {
            return Call::GLOBAL_CALL;
        } else if (proxy->var()->IsLookupSlot()) {
            return Call::LOOKUP_SLOT_CALL;
        }
    }
    
    Property* property = call->AsProperty();
    return property != NULL ? Call::PROPERTY_CALL : Call::OTHER_CALL;
}

void CGObjCJS::VisitCall(Call* node) {
    Expression *callee = node->expression();
    Call::CallType callType = GetCallType(callee, isolate());
    
    if (callType == Call::GLOBAL_CALL) {
        UNIMPLEMENTED();
    } else if (callType == Call::LOOKUP_SLOT_CALL) {
        UNIMPLEMENTED();
    } else if (callType == Call::OTHER_CALL){
        VariableProxy* proxy = callee->AsVariableProxy();
        std::string name = stringFromV8AstRawString(proxy->raw_name());
        
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
        
        bool isJSFunction = !(name == std::string("objcjs_main") || name == std::string("NSLog"));
        if (isJSFunction) {
            Visit(callee);
            llvm::Value *target = PopContext();
           
            auto value = _runtime->invokeJSValue(target, finalArgs);
            PushValueToContext(value);
        } else {
            auto calleeF = _module->getFunction(name);
            PushValueToContext(_builder->CreateCall(calleeF, finalArgs, "calltmp"));
        }
        
    } else if (callType == Call::PROPERTY_CALL){
        Property *property = callee->AsProperty();
       
        Expression *object = property->obj();
        v8::internal::Literal *key = property->key()->AsLiteral();
        
        Visit(key);
        llvm::Value *keyValue = PopContext();
        assert(keyValue);

        llvm::Value *objValue;
        Visit(object);
        objValue = PopContext();
        assert(objValue);
       
        std::string keyName = stringFromV8AstRawString(key->AsRawPropertyName());

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
       
        finalArgs.push_back(ObjcNullPointer());
        //TODO: fix this!
        auto result = _runtime->messageSendProperty(objValue, keyName.c_str(), finalArgs);
        PushValueToContext(result);
    } else {
        UNIMPLEMENTED();
    }
}


void CGObjCJS::VisitCallNew(CallNew* node) {
    Expression *callee = node->expression();
    Call::CallType callType = GetCallType(callee, isolate());
    
    if (callType == Call::GLOBAL_CALL) {
        UNIMPLEMENTED();
    } else if (callType == Call::LOOKUP_SLOT_CALL) {
        UNIMPLEMENTED();
    } else if (callType == Call::OTHER_CALL){
        VariableProxy* proxy = callee->AsVariableProxy();
        std::string name = stringFromV8AstRawString(proxy->raw_name());
        
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
        
        bool isJSFunction = !(name == std::string("objcjs_main") || name == std::string("NSLog"));
       
        assert(isJSFunction);
        
        //Create a new instance and invoke the body
        llvm::Value *targetClass = _runtime->classNamed(name.c_str());
        llvm::Value *target = _runtime->messageSend(targetClass, "new");
      
        _runtime->messageSend(target, "retain");
        
        auto value = _runtime->invokeJSValue(target, finalArgs);
        PushValueToContext(value);
    } else if (callType == Call::PROPERTY_CALL){
        Property *property = callee->AsProperty();
       
        VariableProxy *object = property->obj()->AsVariableProxy();
        v8::internal::Literal *key = property->key()->AsLiteral();
        
        Visit(key);
        llvm::Value *keyValue = PopContext();
        assert(keyValue);

        Visit(object);
        llvm::Value *objValue = PopContext();
        assert(objValue);

        std::string keyName = stringFromV8AstRawString(key->AsRawPropertyName());

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
       
        finalArgs.push_back(ObjcNullPointer());
        //TODO: fix this!
        auto result = _runtime->messageSendProperty(objValue, keyName.c_str(), finalArgs);
        PushValueToContext(result);
    } else {
        UNIMPLEMENTED();
    }
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
 
    const char *assignOpSelector = node->op() == Token::INC ?
                "objcjs_increment" : "objcjs_decrement";

  
    llvm::AllocaInst *alloca;
    if (assign_type == VARIABLE){
        VariableProxy *proxy =  node->expression()->AsVariableProxy();
        alloca = _context->valueForKey(stringFromV8AstRawString(proxy->raw_name()));
        EmitVariableLoad(proxy);
    } else if (assign_type == NAMED_PROPERTY) {
        UNIMPLEMENTED();
    } else {
        UNIMPLEMENTED();
    }
    
    auto value = PopContext();
    
    if (node->is_prefix()) {
        auto result = _runtime->messageSend(value, assignOpSelector);
        _builder->CreateStore(result, alloca);
        
        PushValueToContext(result);
    }
    
    Visit(node->expression());
    PopContext();
    
    if (node->is_postfix()) {
        PushValueToContext(value);

        auto result = _runtime->messageSend(value, assignOpSelector);
        _builder->CreateStore(result, alloca);
    }
}

void CGObjCJS::VisitBinaryOperation(BinaryOperation* expr) {
  switch (expr->op()) {
      case Token::COMMA:
          return VisitComma(expr);
      case Token::OR:
          return EmitLogicalOr(expr);
      case Token::AND:
          return EmitLogicalAnd(expr);
      default:
          return VisitArithmeticExpression(expr);
  }
}

void CGObjCJS::VisitComma(BinaryOperation* expr) {
    UNIMPLEMENTED();
}

void CGObjCJS::EmitLogicalAnd(BinaryOperation *expr) {
    auto function = _builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *trueBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "and.true", function);
    llvm::BasicBlock *falseBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "and.false");
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "and.merge");
    
    Visit(expr->left());
    auto left = PopContext();
    
    // convert condition to a bool by comparing equal to NullPointer.
    auto condV = _builder->CreateICmpEQ(_runtime->boolValue(left), ObjcNullPointer());
    _builder->CreateCondBr(condV, falseBB, trueBB);
    _builder->SetInsertPoint(trueBB);
    
    Visit(expr->right());
    auto right = PopContext();
    
    _builder->CreateBr(mergeBB);
    trueBB = _builder->GetInsertBlock();
    
    function->getBasicBlockList().push_back(falseBB);

    //emit false block
    _builder->SetInsertPoint(falseBB);
    _builder->CreateBr(mergeBB);
    
    //emit merge block
    function->getBasicBlockList().push_back(mergeBB);
    _builder->SetInsertPoint(mergeBB);
    
    llvm::PHINode *ph = _builder->CreatePHI(ObjcPointerTy(), 2, "and.value");
    ph->addIncoming(right, trueBB);
    ph->addIncoming(left, falseBB);
    PushValueToContext(ph);
}

void CGObjCJS::EmitLogicalOr(BinaryOperation *expr) {
    auto function = _builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *trueBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "or.true", function);
    llvm::BasicBlock *falseBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "or.false");
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "or.merge");
    
    Visit(expr->left());
    auto left = PopContext();
    
    // convert condition to a bool by comparing equal to NullPointer.
    auto condV = _builder->CreateICmpEQ(_runtime->boolValue(left), ObjcNullPointer());
    _builder->CreateCondBr(condV, falseBB, trueBB);
    
    _builder->SetInsertPoint(trueBB);
    
    _builder->CreateBr(mergeBB);
    trueBB = _builder->GetInsertBlock();
    
    //emit false block
    function->getBasicBlockList().push_back(falseBB);
    _builder->SetInsertPoint(falseBB);
    Visit(expr->right());
    auto right = PopContext();
    _builder->CreateBr(mergeBB);
    
    //emit merge block
    function->getBasicBlockList().push_back(mergeBB);
    _builder->SetInsertPoint(mergeBB);
    
    llvm::PHINode *ph = _builder->CreatePHI(ObjcPointerTy(), 2, "or.value");
    ph->addIncoming(left, trueBB);
    ph->addIncoming(right, falseBB);
    PushValueToContext(ph);
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
   
    std::vector<llvm::Value *>Args;
    Args.push_back(lhs);
    Args.push_back(rhs);
    
    auto selector = opSelectorByToken[expr->op()].c_str();
    assert(selector && "unsupported arithmatic operation");
   
    auto result = _runtime->messageSend(lhs, selector, rhs);
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
    }
    
    if (expr->IsLiteralCompareUndefined(&sub_expr, isolate())) {
        UNIMPLEMENTED();
    }
    
    if (expr->IsLiteralCompareNull(&sub_expr)) {
        UNIMPLEMENTED();
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
    ILOG("enter context");
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

//TODO : ARC
void CGObjCJS::VisitStartStackAccumulation(AstNode *expr) {
    assert(0);
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

//SymbolIsClass - this symbol is currently known as a class
bool CGObjCJS::SymbolIsClass(std::string symbol) {
    return _module->getFunction(variableName);
}
