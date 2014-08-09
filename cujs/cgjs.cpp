//
//  codegen.cpp
//  cujs
//
//  Created by Jerry Marino on 7/6/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include <src/scopes.h>
#include <src/zone.h>

#include <llvm-c/Core.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CFG.h>

#include "cgjs.h"
#include "cgjsruntime.h"
#include "cgclang.h"
#include "cgjsmacrovisitor.h"
#include "cujsutils.h"

#define CGJSDEBUG 0
#define ILOG(A, ...) if (CGJSDEBUG){ printf(A,##__VA_ARGS__), printf("\n");}

using namespace v8::internal;
using namespace cujs;
using namespace llvm;

class cujs::CGContext {
public:
    Scope *_scope;
    std::vector<llvm::Value *> _context;
    std::map<std::string, llvm::AllocaInst*> _namedValues;
    
    CGContext *Extend(){
        auto extended = new CGContext();
        return extended;
    }
  
    ~CGContext(){ };
    
    void dump(){
        for (unsigned i = 0; i < _context.size(); i++){
            _context[i]->dump();
        }
    }
    
    size_t size(){
        return _context.size();
    }
    
    void setValue(std::string key, llvm::AllocaInst *value) {
        if (_namedValues[key]){
            std::cout << "waring: tried to set existing value for key" << key << "\n";
            return;
        }
        _namedValues[key] = value;
    }
   
    llvm::AllocaInst *valueForKey(std::string key) {
        return _namedValues[key];
    }
    
    llvm::AllocaInst *lookup(std::vector<CGContext *>contexts, std::string key){
        llvm::AllocaInst *value = NULL;
        for (size_t i = contexts.size()-1; i; i--){
            auto foundValue = contexts[i]->valueForKey(key);
            if (foundValue){
                value = foundValue;
                break;
            }
        }
        return value;
    }
    
    void Push(llvm::Value *value) {
        if (value) {
            _context.push_back(value);
        } else {
            printf("warning: tried to push null value \n");
        }   
    }
    
    void EmptyStack(){
        while (size()) {
            Pop();
        }
    }
    
    llvm::Value *Pop(){
        if (!size()) {
            return NULL;
        }
        llvm::Value *value = _context.back();
        _context.pop_back();
        return value;
    };
};

#define CALL_LOG(STR)\
    _builder->CreateCall(_module->getFunction("NSLog"), _runtime->newString(STR))

std::string cujs::stringFromV8AstRawString(const AstRawString *raw) {
    std::string str;
    size_t size = raw->length();
    const unsigned char *data = raw->raw_data();
    for (int i = 0; i < size; i++) {
        str += data[i];
    }
    return str;
}

enum LhsKind { VARIABLE, NAMED_PROPERTY, KEYED_PROPERTY };

LhsKind GetLhsKind(Expression *node){
    LhsKind assignType = VARIABLE;
    Property *property = node->AsProperty();
    
    if (property != NULL) {
        assignType = (property->key()->IsPropertyName())
        ? NAMED_PROPERTY
        : KEYED_PROPERTY;
    }
    
    return assignType;
}

static std::string FUNCTION_CMD_ARG_NAME("_cmd");
static std::string FUNCTION_THIS_ARG_NAME("this");

static std::string DEFAULT_RET_ALLOCA_NAME("DEFAULT_RET_ALLOCA");
static std::string SET_RET_ALLOCA_NAME("SET_RET_ALLOCA");

#pragma mark - CGJS

CGJS::CGJS(std::string name,
                   CompilationInfoWithZone *info)
{

    _info = info;
    InitializeAstVisitor(info->zone());
    
    _name = &name;
    llvm::LLVMContext &Context = llvm::getGlobalContext();
    _builder = new llvm::IRBuilder<> (Context);
    _module = new llvm::Module(name, Context);
    _context = NULL;
    _runtime = new CGJSRuntime(_builder, _module);
    
    assignOpSelectorByToken[Token::ASSIGN_ADD] = std::string("cujs_add:");
    assignOpSelectorByToken[Token::ASSIGN_SUB] = std::string("cujs_subtract:");
    assignOpSelectorByToken[Token::ASSIGN_MUL] = std::string("cujs_multiply:");
    assignOpSelectorByToken[Token::ASSIGN_DIV] = std::string("cujs_divide:");
    assignOpSelectorByToken[Token::ASSIGN_MOD] = std::string("cujs_mod:");
    
    assignOpSelectorByToken[Token::ASSIGN_BIT_OR] = std::string("cujs_bitor:");
    assignOpSelectorByToken[Token::ASSIGN_BIT_XOR] = std::string("cujs_bitxor:");
    assignOpSelectorByToken[Token::ASSIGN_BIT_AND] = std::string("cujs_bitand:");
    
    assignOpSelectorByToken[Token::ASSIGN_SHL] = std::string("cujs_shiftleft:");
    assignOpSelectorByToken[Token::ASSIGN_SAR] = std::string("cujs_shiftright:");
    assignOpSelectorByToken[Token::ASSIGN_SHR] = std::string("cujs_shiftrightright:");
    
    opSelectorByToken[Token::ADD] = std::string("cujs_add:");
    opSelectorByToken[Token::SUB] = std::string("cujs_subtract:");
    opSelectorByToken[Token::MUL] = std::string("cujs_multiply:");
    opSelectorByToken[Token::DIV] = std::string("cujs_divide:");
    opSelectorByToken[Token::MOD] = std::string("cujs_mod:");
    
    opSelectorByToken[Token::BIT_OR] = std::string("cujs_bitor:");
    opSelectorByToken[Token::BIT_XOR] = std::string("cujs_bitxor:");
    opSelectorByToken[Token::BIT_AND] = std::string("cujs_bitand:");
    
    opSelectorByToken[Token::SHL] = std::string("cujs_shiftleft:");
    opSelectorByToken[Token::SAR] = std::string("cujs_shiftright:");
    opSelectorByToken[Token::SHR] = std::string("cujs_shiftrightright:");
}

CGJS::~CGJS() {
    delete _builder;
    delete _module;
    delete _runtime;
};

void CGJS::Codegen() {
    auto moduleFunction = ObjcCodeGenModuleInit(_builder, _module, *_name);
    SetModuleCtor(_module, moduleFunction);

    //start module inserting at beginning of init function
    _builder->SetInsertPoint(&moduleFunction->getBasicBlockList().front());
    
    auto macroVisitor = cujs::CGJSMacroVisitor(this, zone());
    macroVisitor.Visit(_info->function());
    _macros = macroVisitor._macros;   

    Visit(_info->function());
}

void CGJS::Dump(){
    _module->dump();
}

void CGJS::CreateArgumentAllocas(llvm::Function *F, v8::internal::Scope *scope) {
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

void CGJS::CreateJSArgumentAllocas(llvm::Function *F, v8::internal::Scope *scope) {
    llvm::Function::arg_iterator AI = F->arg_begin();
    llvm::Value *argSelf = AI++;
    int numParams = scope->num_parameters();
   
    NewLocalStringVar(FUNCTION_THIS_ARG_NAME, _module);
    llvm::AllocaInst *allocaThis = CreateEntryBlockAlloca(F, FUNCTION_THIS_ARG_NAME);
    _builder->CreateStore(argSelf, allocaThis);
    _context->setValue(FUNCTION_THIS_ARG_NAME, allocaThis);
   
    NewLocalStringVar(FUNCTION_CMD_ARG_NAME, _module);
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

void CGJS::VisitDeclarations(ZoneList<Declaration*> *declarations){
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

void CGJS::VisitStatements(ZoneList<Statement*> *statements){
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

void CGJS::VisitBlock(Block *block){
    VisitStatements(block->statements());
}

void CGJS::VisitVariableDeclaration(v8::internal::VariableDeclaration *node) {
    if(IsInGlobalScope()){
        VariableProxy *var = node->proxy();
        auto insertPt = _builder->GetInsertBlock();
        std::string globalName = stringFromV8AstRawString(var->raw_name());
        _runtime->declareGlobal(globalName);
        _builder->SetInsertPoint(insertPt);
        return;
    }
    
    std::string name = stringFromV8AstRawString(node->proxy()->raw_name());
    llvm::Function *function = _builder->GetInsertBlock()->getParent();
    llvm::AllocaInst *alloca = CreateEntryBlockAlloca(function, name);
    _context->setValue(name, alloca);

    VariableProxy *var = node->proxy();
    Visit(var);
}

void CGJS::VisitFunctionDeclaration(v8::internal::FunctionDeclaration *node) {
    Visit(node->fun());
}

void CGJS::EmitFunctionPrototype(v8::internal::FunctionLiteral *node) {
    auto anonName = _nameByFunctionID[node->id().ToInt()];
    std::string name = anonName.length() ? anonName : stringFromV8AstRawString(node->raw_name());
    assert(name.length() && "missing function name");

    bool isJSFunction = SymbolIsJSFunction(name);
    if (isJSFunction) {
        assert(!_module->getFunction(name) && "function is already declared");
    }
    
    int numParams = node->scope()->num_parameters();
    if (isJSFunction){
        //Add define JSFunction to front of the module constructor
        llvm::Function *moduleCtor = _module->getFunction(*_name);
        assert(moduleCtor);
        llvm::BasicBlock *moduleCtorBB = &moduleCtor->getBasicBlockList().front();
        llvm::Instruction *defineFunction = (llvm::Instruction *)_runtime->defineJSFuction(name.c_str(), numParams);
        moduleCtorBB->getInstList().push_front(defineFunction);

        //If this is a nested declaration set the parent to this!
        if (_builder->GetInsertBlock() &&
            _builder->GetInsertBlock()->getParent() &&
            !(_builder->GetInsertBlock()->getParent()->getName().equals(*_name))) {
            auto functionClass = _runtime->classNamed(name.c_str());
            auto jsThisAlloca = _context->valueForKey(FUNCTION_THIS_ARG_NAME);
            if (jsThisAlloca) {
                auto jsThis = _builder->CreateLoad(jsThisAlloca, "load-this");
                _runtime->messageSend(functionClass, "_cujs_setParent:", jsThis);
            } else {
                ILOG("warning missing this.. something is broken!");
            }
        }
    }
    
    auto functionAlloca = _builder->CreateAlloca(ObjcPointerTy());
    _builder->CreateStore(_runtime->classNamed(name.c_str()), functionAlloca);
    _context->setValue(name, functionAlloca);
}

#pragma mark - Modules

void CGJS::VisitModuleDeclaration(ModuleDeclaration *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitImportDeclaration(ImportDeclaration *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitExportDeclaration(ExportDeclaration *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitModuleLiteral(ModuleLiteral *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitModuleVariable(ModuleVariable *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitModulePath(ModulePath *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitModuleUrl(ModuleUrl *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitModuleStatement(ModuleStatement *node) {
    UNIMPLEMENTED();
}

#pragma mark - Statements

void CGJS::VisitExpressionStatement(ExpressionStatement *node) {
    Visit(node->expression());
}

void CGJS::VisitEmptyStatement(EmptyStatement *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitIfStatement(IfStatement *node) {
    CGIfStatement(node, true);
}

void CGJS::CGIfStatement(IfStatement *node, bool flag){
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
void CGJS::VisitContinueStatement(ContinueStatement *node) {
    auto currentBlock = _builder->GetInsertBlock();
    auto function = _builder->GetInsertBlock()->getParent();
    bool start = false;
    llvm::BasicBlock *jumpTarget = NULL;
    for (llvm::Function::iterator BB = function->end(), e = function->begin(); BB != e; --BB){
        if (BB == *currentBlock) {
            start = true;
        }
        
        if (start && BB->getName().startswith(llvm::StringRef("loop.next"))) {
            jumpTarget = BB;
            break;
        }
    }
  
    if (!jumpTarget) {
        assert(0 && "invalid continue statement");
    }
    
    _builder->CreateBr(jumpTarget);
}

void CGJS::VisitBreakStatement(BreakStatement *node) {
    auto currentBlock = _builder->GetInsertBlock();
    auto function = _builder->GetInsertBlock()->getParent();
    
    bool start = false;
    llvm::BasicBlock *jumpTarget = NULL;
    for (llvm::Function::iterator BB = function->end(), e = function->begin(); BB != e; --BB){
        if (BB == *currentBlock) {
            start = true;
        }
        
        if (start && BB->getName().startswith(llvm::StringRef("loop.after"))) {
            jumpTarget = BB;
            break;
        }
    }
  
    if (!jumpTarget) {
        assert(0 && "invalid break statement");
    }
    
    _builder->CreateBr(jumpTarget);
}

//Insert the expressions into a returning block
void CGJS::VisitReturnStatement(ReturnStatement *node) {
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

void CGJS::VisitWithStatement(WithStatement *node) {
    UNIMPLEMENTED(); //Deprecated
}

#pragma mark - Switch

void CGJS::VisitSwitchStatement(SwitchStatement *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitCaseClause(CaseClause *clause) {
    UNIMPLEMENTED();
}

#pragma mark - Loops

void CGJS::VisitWhileStatement(WhileStatement *node) {
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

void CGJS::VisitDoWhileStatement(DoWhileStatement *node) {
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

void CGJS::VisitForStatement(ForStatement *node) {
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

void CGJS::VisitForInStatement(ForInStatement *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitForOfStatement(ForOfStatement *node) {
    UNIMPLEMENTED();
}

#pragma mark - Try
void CGJS::VisitTryCatchStatement(TryCatchStatement *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitTryFinallyStatement(TryFinallyStatement *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitDebuggerStatement(DebuggerStatement *node) {
    UNIMPLEMENTED();
}

static void CleanupInstructionsAfterBreaks(llvm::Function *function){
    for (llvm::Function::iterator BB = function->begin(), e = function->end(); BB != e; ++BB){
        bool didBreak = false;
        for (llvm::BasicBlock::iterator i = BB->begin(), h = BB->end();  i && i != h; ++i){
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

//Define the body of the function
void CGJS::VisitFunctionLiteral(v8::internal::FunctionLiteral *node) {
    auto startIB = _builder->GetInsertBlock();
    auto name = stringFromV8AstRawString(node->raw_name());
    if (!_context){
        EnterContext();
       //start module inserting at beginning of init function
        auto moduleFunction = _module->getFunction(*_name);
        _builder->SetInsertPoint(&moduleFunction->getBasicBlockList().front());
       
        auto _BB = _builder->GetInsertBlock();
        VisitDeclarations(node->scope()->declarations());
        _builder->SetInsertPoint(_BB);
        VisitStatements(node->body());
        _builder->SetInsertPoint(_BB);
        EndAccumulation();

        //add terminator to module function
        llvm::BasicBlock *BB = &moduleFunction->getBasicBlockList().front();
        llvm::ReturnInst::Create(_module->getContext(), ObjcNullPointer(), BB);
        return;
    }

    if (!name.length()) {
        name = string_format("JSFUNC_%s", std::to_string(random()).c_str());
        _nameByFunctionID[node->id().ToInt()] = name;
        _runtime->newString(name);
        NewLocalStringVar(name, _module);

        EmitFunctionPrototype(node);
    }
    
    auto function = _module->getFunction(name);
  
    if (!function) {
        EmitFunctionPrototype(node);
        function = _module->getFunction(name);
        EnterContext();
    } else {
        EnterContext();
        
    }
    
    // Create a new basic block to start insertion into.
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", function);
    _builder->SetInsertPoint(BB);
   
    bool isJSFunction = SymbolIsJSFunction(name);
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
   
    // This code will not be executed and llvm will throw
    // errors if it exists, in the best case it wouldn't be added
    // in the first place
    CleanupInstructionsAfterBreaks(function);

    llvm::BasicBlock *front = &function->front();
    auto needsTerminator = !front->getTerminator();
    if (needsTerminator) {
        front->getInstList().push_back(llvm::BranchInst::Create(setRetBB));
    }
    
    EndAccumulation();
    
    // Return to starting basic block and push
    // this function in case of assignment
    _builder->SetInsertPoint(startIB);
    PushValueToContext(_runtime->classNamed(name.c_str()));
}

void CGJS::VisitNativeFunctionLiteral(NativeFunctionLiteral *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitConditional(Conditional *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitLiteral(class Literal *node) {
    CGLiteral(node->value(), true);
}

#pragma mark - Literals

// String can be in two-byte encoding, and as a result non-ASCII characters
// will be ignored in the output.
char *ToAsciiArray(v8::internal::String *string) {
  // Static so that subsequent calls frees previously allocated space.
  // This also means that previous results will be overwritten.
  static char *buffer = NULL;
  if (buffer != NULL) free(buffer);
  buffer = new char[string->length()+1];
  string->WriteToFlat(string, reinterpret_cast<uint8_t*>(buffer), 0, string->length());
  buffer[string->length()] = 0;
  return buffer;
}

std::string StringWithV8String(v8::internal::String *string) {
    char *ascii = ToAsciiArray(string);
    return std::string(ascii);
}

llvm::Value *CGJS::CGLiteral(Handle<Object> value, bool push) {
    Object *object = *value;
    llvm::Value *lvalue = NULL;
    if (object->IsString()) {
        String *string = String::cast(object);
        auto name = StringWithV8String(string);
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
            unsigned long len = (unsigned long)JSArray::cast(object)->length();
            ILOG("JS-array[%lul]", len);
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

void CGJS::VisitRegExpLiteral(RegExpLiteral *node) {
    UNIMPLEMENTED();
}

__attribute__((unused))
llvm::Value *makeKeyNameValue(Expression *key, llvm::Module *module){
    std::string keyStringValue;
    if (key->IsPropertyName()) {
        keyStringValue = cujs::stringFromV8AstRawString(((Literal *)key)->AsRawPropertyName());
    }
    
    if(key->IsLiteral()){
        auto literalKey = key->AsLiteral();
        keyStringValue = std::to_string(literalKey->value()->Number());
    }
   
    return NewLocalStringVar(keyStringValue, module);
    
    if (!keyStringValue.length()) {
        UNIMPLEMENTED();
    }
    return NULL;
}

std::string makeKeyName(Expression *key, llvm::Module *module){
    if (key->IsPropertyName()) {
        return cujs::stringFromV8AstRawString(((Literal *)key)->AsRawPropertyName());
    }
    
    if(key->IsLiteral()){
        auto literalKey = key->AsLiteral();
        auto str = std::to_string(literalKey->value()->Number());
        NewLocalStringVar(str, module);
        return str;
    }
    
    UNIMPLEMENTED();
    return NULL;
}

void CGJS::VisitObjectLiteral(ObjectLiteral *node) {
    auto newObject = _runtime->newObject();
    
    for (int i = 0; i < node->properties()->length(); i++) {
        ObjectLiteral::Property *property = node->properties()->at(i);

        Visit(property->value());
        auto value = PopContext();
        
        Visit(property->key());
        auto keyValue = PopContext();
       
        //TODO : wrap this up
        EmitKeyedPropertyAssignment(newObject, keyValue, value);
        PopContext();
    }
    
    PushValueToContext(newObject);
}

void CGJS::VisitArrayLiteral(ArrayLiteral *node) {
    auto newObject = _runtime->newObject();

    for (int i = 0; i < node->values()->length(); i++) {
        Visit(node->values()->at(i));
        auto value = PopContext();

        auto keyValue = _runtime->newString(std::to_string(i));
        EmitKeyedPropertyAssignment(newObject, keyValue, value);
    }
    
    PushValueToContext(newObject);
}

void CGJS::VisitVariableProxy(VariableProxy *node) {
    EmitVariableLoad(node);
}

void CGJS::VisitAssignment(Assignment *node) {
    assert(node->target()->IsValidReferenceExpression());
    LhsKind assignType = GetLhsKind(node->target());

    auto BB = _builder->GetInsertBlock();
    Visit(node->value());
    _builder->SetInsertPoint(BB);
    
    llvm::Value *value = PopContext();
    switch (assignType) {
        case VARIABLE: {
            VariableProxy *targetProxy = (VariableProxy *)node->target();
            assert(targetProxy && "target for assignment required");
            assert(value && "missing value - not implemented");

            std::string targetName = stringFromV8AstRawString(targetProxy->raw_name());
            if (node->op() !=Token::ASSIGN &&
                node->op() != Token::EQ &&
                node->op() != Token::INIT_VAR){
                EmitVariableLoad(targetProxy);
                auto target = PopContext();
                auto selector = assignOpSelectorByToken[node->op()].c_str();
                assert(selector && "unsupported assignment operation");
                value = _runtime->messageSend(target, selector, value);
            }
            
            if (node->op() == Token::INIT_VAR) {
                //FIXME:
                ILOG("INIT_VAR %s", targetName.c_str());
            }

            EmitVariableStore(targetProxy, value);
            break;
        } case NAMED_PROPERTY: {
            Property *property = node->target()->AsProperty();
            if (!value){
                ILOG("Waring - missing value");
                abort();
            }
            
            EmitNamedPropertyAssignment(property, value);
            break;
        }
        case KEYED_PROPERTY: {
            UNIMPLEMENTED();
            break;
        }
    }
}

void CGJS::EmitKeyedPropertyAssignment(llvm::Value *target, llvm::Value *key, llvm::Value *value){
    std::vector<llvm::Value *>args;
    args.push_back(value);
    args.push_back(key);
    PushValueToContext(_runtime->messageSend(target, "cujs_ss_setValue:forKey:", args));
}

void CGJS::EmitNamedPropertyAssignment(Property *property, llvm::Value *value){
    Expression *object = property->obj();
    v8::internal::Literal *key = property->key()->AsLiteral();
    auto BB = _builder->GetInsertBlock();
    assert(BB);
    
    Visit(key);
    llvm::Value *keyValue = PopContext();
    assert(keyValue);
    
    _builder->SetInsertPoint(BB);
    Visit(object);
    llvm::Value *objValue = PopContext();
    assert(objValue);
    
    _builder->SetInsertPoint(BB);
    std::string keyName = stringFromV8AstRawString(key->AsRawPropertyName());

    //send to property
    ObjCMethod *method = ((ObjCMethod *)_runtime->_objCMethodBySelector[keyName]);
    if (method && method->name.length()) {
        keyName = method->name;
    }
    
    _runtime->assignProperty(objValue, keyName, value);
}

void CGJS::EmitVariableStore(VariableProxy *targetProxy, llvm::Value *value) {
    assert(targetProxy && "target for assignment required");
    assert(value && "missing value - not implemented");
    std::string targetName = stringFromV8AstRawString(targetProxy->raw_name());
    
    bool scopeHasTarget = _context->valueForKey(targetName);
    if (scopeHasTarget){
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
            _runtime->messageSend(jsThis, "_cujs_env_setValue:declareKey:", Args);
        }
        
    } else if (_context->lookup(_contexts, targetName)){
        llvm::AllocaInst *targetAlloca = _builder->CreateAlloca(ObjcPointerTy());
        //Set the environment value
        auto jsThisAlloca = _context->valueForKey(FUNCTION_THIS_ARG_NAME);
        auto jsThis = _builder->CreateLoad(jsThisAlloca);
        
        _builder->CreateStore(_runtime->newString(targetName), targetAlloca);
        auto keypathStringValue = _builder->CreateLoad(targetAlloca);
        
        std::vector<llvm::Value *>Args;
        Args.push_back(value);
        Args.push_back(keypathStringValue);
        _runtime->messageSend(jsThis, "_cujs_env_setValue:forKey:", Args);
    } else if (_module->getGlobalVariable(llvm::StringRef(targetName), false)) {
        llvm::GlobalVariable *global = _module->getGlobalVariable(llvm::StringRef(targetName));
        assert(!global->isConstant());
        _builder->CreateStore(value, global);
    } else {
        UNIMPLEMENTED();
    }
}

void CGJS::EmitVariableLoad(VariableProxy *node) {
    std::string variableName = stringFromV8AstRawString(node->raw_name());
    auto varAlloca = _context->valueForKey(variableName);
    
    if (SymbolIsClass(variableName) && !varAlloca){
        PushValueToContext(_runtime->classNamed(variableName.c_str()));
        return;
    }
    
    if (varAlloca) {
        PushValueToContext(_builder->CreateLoad(varAlloca, variableName));
        return;
    }
    
    auto nestedVarAlloc = _context->lookup(_contexts, variableName);
    if (nestedVarAlloc) {
        //NTS: always create an alloca for an arugment you want to pass to a function!!
        auto environmentVariableAlloca = _builder->CreateAlloca(ObjcPointerTy(), 0, "env_var_alloca");
        _builder->CreateStore(_runtime->newString(variableName), environmentVariableAlloca);
        auto keypathArgument = _builder->CreateLoad(environmentVariableAlloca);
        
        //load the value from the parents environment
        auto parentFunction = _builder->GetInsertBlock()->getParent();
        auto parentName = parentFunction ->getName().str();
        auto parentThis = _runtime->messageSend(_runtime->classNamed(parentName.c_str()), "_cujs_parent");
        auto value = _runtime->messageSend(parentThis, "_cujs_env_valueForKey:", keypathArgument);
        
        PushValueToContext(value);
        return;
    }
    
    auto global = _module->getGlobalVariable(llvm::StringRef(variableName));
    if (global) {
        PushValueToContext(_builder->CreateLoad(global));
        return;
    }
    
    UNIMPLEMENTED();
}

void CGJS::VisitYield(Yield *node) {
    UNIMPLEMENTED(); //Deprecated
}

void CGJS::VisitThrow(Throw *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitProperty(Property *node) {
    Property *property = node;
    
    Expression *object = property->obj();
    Expression *key = property->key();
    
    Visit(key);
    llvm::Value *keyValue = PopContext();
    assert(keyValue);
   
    Visit(object);
    llvm::Value *objValue = PopContext();
    assert(objValue);
  
    LhsKind type = GetLhsKind(property);
    
    if (type == NAMED_PROPERTY) {
        std::string keyName = makeKeyName(property->key(), _module);
        PushValueToContext(_runtime->messageSend(objValue, keyName.c_str()));
    } else if (type == KEYED_PROPERTY) {
        PushValueToContext(_runtime->messageSend(objValue, "cujs_ss_valueForKey:", keyValue));
    } else {
        UNIMPLEMENTED();
    }
}

static Call::CallType GetCallType(Expression *call, Isolate *isolate) {
    VariableProxy *proxy = call->AsVariableProxy();
    if (proxy != NULL && proxy->var()) {
        
        if (proxy->var()->IsUnallocated()) {
            return Call::GLOBAL_CALL;
        } else if (proxy->var()->IsLookupSlot()) {
            return Call::LOOKUP_SLOT_CALL;
        }
    }
    
    Property *property = call->AsProperty();
    return property != NULL ? Call::PROPERTY_CALL : Call::OTHER_CALL;
}


void CGJS::EmitStructLoadCall(Call *node) {
    ZoneList<Expression*> *args = node->arguments();
    auto argExpr = args->at(0);
    auto nameStr = argExpr->AsLiteral()->raw_value()->AsString();
    if (!nameStr) {
        return;
    }
    _builder->CreateCall(_module->getFunction("NSLog"), _runtime->newString("Make struct call"));
    
    //Get struct type
    std::string name = stringFromV8AstRawString(nameStr);

    //The objc_Struct macro only takes 1 argument
    auto argOneExpr = args->at(1);
    Visit(argOneExpr);


    llvm::CallInst *objcPointerArgValue = (llvm::CallInst *)PopContext();
    EmitStructLoadCast(name, objcPointerArgValue);
}

void CGJS::EmitStructLoadCast(std::string name, llvm::CallInst *objcPointerArgValue) {
    CGJSRuntime *runtime = _runtime;

    //TODO : nicely declare struct names
    std::string defName = name;
    ObjCStruct *objCStructTy = runtime->_objCStructByName[name];
    assert(objCStructTy && "Missing type for struct");
    
    // Type Definitions
    auto IB = _builder->GetInsertBlock();
    llvm::Type *StructTy = _module->getTypeByName(defName);
   
    // The type to cast msgSend to
    std::vector<llvm::Type*>castedMsgSendArgumentTypes;
    castedMsgSendArgumentTypes.push_back(ObjcPointerTy());
    FunctionType *castedMsgSendFunctionTy = FunctionType::get(
                                                StructTy,
                                                castedMsgSendArgumentTypes,
                                                true);

    PointerType *castedMsgSendPointerTy = PointerType::get(castedMsgSendFunctionTy, 0);

    // Depending on the current ABI, structs will be returned on
    // different addresses and require a specifc msg send
    std::string msgSendFuncName = objCStructTy->size <= 8 ? "objc_msgSend" : "objc_msgSend_stret";
  
    Constant *msgSendFunc = ConstantExpr::getCast(Instruction::BitCast, _module->getFunction(msgSendFuncName), castedMsgSendPointerTy);
    AllocaInst *ptr_castedMsgSendFunc = _builder->CreateAlloca(castedMsgSendPointerTy);
    _builder->CreateStore(msgSendFunc, ptr_castedMsgSendFunc);

    // Add the original value, which could only be derived from
    // an objc msg send and add them to the args of the new call
    std::vector<Value*> castedMsgSendArgs;
    auto opers = objcPointerArgValue->getNumArgOperands();
    for (unsigned i = 0; i < opers; i++) {
        llvm::Value *arg = objcPointerArgValue->getArgOperand(i);
        castedMsgSendArgs.push_back(arg);
    }

    //Remove the wrongly typed value
    objcPointerArgValue->eraseFromParent();
   
    _builder->CreateCall(_module->getFunction("NSLog"), _runtime->newString("Invoke struct casted msgSend"));
    
    //Call casted function
    auto castedMsgSend = _builder->CreateLoad(ptr_castedMsgSendFunc);
    CallInst *castedMsgSendCallResult = _builder->CreateCall(castedMsgSend, castedMsgSendArgs);

    _builder->CreateCall(_module->getFunction("NSLog"), _runtime->newString("Did invoke struct casted msgSend"));
  
    AllocaInst *ptr_resultOfCastedCall = _builder->CreateAlloca(StructTy);
    PointerType *PointerTy_18 = PointerType::get(StructTy, 0);
    CastInst *ptr_42 = new BitCastInst(ptr_resultOfCastedCall, PointerTy_18, "", IB);
 
    _builder->CreateStore(castedMsgSendCallResult, ptr_resultOfCastedCall);
    
    //Create a struct instance with this name
    std::vector<llvm::Value *> Args;
    Args.push_back(_runtime->newString(objCStructTy->name));
    Args.push_back(ptr_42);

    auto function = _module->getFunction("objc_Struct");
    assert(function);
    auto result = _builder->CreateCall(function,  Args);
    PushValueToContext(result);
}

void CGJS::VisitCall(Call *node) {
    Expression *callee = node->expression();
    Call::CallType callType = GetCallType(callee, isolate());
    
    if (callType == Call::GLOBAL_CALL) {
        UNIMPLEMENTED();
    } else if (callType == Call::LOOKUP_SLOT_CALL) {
        UNIMPLEMENTED();
    } else if (callType == Call::OTHER_CALL){
        std::string name;
        VariableProxy *proxy = callee->AsVariableProxy();
        if (proxy) {
            name = stringFromV8AstRawString(proxy->raw_name());

            // handle special case compile time magic
            auto macro = _macros[name];
            if (macro){
                macro(this, node);
                return;
            }
            
            if (name == "objc_Struct") {
                EmitStructLoadCall(node);
                return;
            }
            
            if (_runtime->_structs.count(name)) {
                //TODO:
                return;
            }
        } else {
            // handle anon function calls
            // implements (function(){}())
            assert(callee->AsFunctionLiteral());
            name = _nameByFunctionID[node->id().ToInt()];
        }
        
        ZoneList<Expression*> *args = node->arguments();
        for (int i = 0; i <args->length(); i++) {
            Visit(args->at(i));
        }
        
        std::vector<llvm::Value *> finalArgs;
        for (unsigned i = 0; i < args->length(); i++){
            llvm::Value *arg = PopContext();
            assert(arg);
            finalArgs.push_back(arg);
        }
        std::reverse(finalArgs.begin(), finalArgs.end());
        
        bool isJSFunction = SymbolIsJSFunction(name);
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
        EmitPropertyCall(callee, node->arguments());
    } else {
        UNIMPLEMENTED();
    }
}

void CGJS::EmitPropertyCall(Expression *callee, ZoneList<Expression*> *args){
    Property *property = callee->AsProperty();
    
    Expression *object = property->obj();
    v8::internal::Literal *key = property->key()->AsLiteral();
    
    Visit(key);
    llvm::Value *keyValue = PopContext();
    assert(keyValue);
    
    Visit(object);
    llvm::Value *objValue = PopContext();
    assert(objValue);
    
    std::string keyName = stringFromV8AstRawString(key->AsRawPropertyName());

    //send to property
    ObjCMethod *method = ((ObjCMethod *)_runtime->_objCMethodBySelector[keyName.c_str()]);
    if (method) {
        keyName = method->name;
    }
    
    llvm::Value *result = _runtime->messageSend(objValue, keyName.c_str(), makeArgs(args));
    PushValueToContext(result);
}

std::vector <llvm::Value *>CGJS::makeArgs(ZoneList<Expression*> *args) {
    std::vector<llvm::Value *> finalArgs;
    
    if (args->length() == 0) {
        return finalArgs;
    }
    
    for (int i = 0; i <args->length(); i++) {
        Visit(args->at(i));
    }
    
    for (unsigned i = 0; i < args->length(); i++){
        llvm::Value *arg = PopContext();
        finalArgs.push_back(arg);
    }
    std::reverse(finalArgs.begin(), finalArgs.end());
    return finalArgs;
}

void CGJS::VisitCallNew(CallNew *node) {
    Expression *callee = node->expression();
    Call::CallType callType = GetCallType(callee, isolate());
    
    if (callType == Call::GLOBAL_CALL) {
        UNIMPLEMENTED();
    } else if (callType == Call::LOOKUP_SLOT_CALL) {
        UNIMPLEMENTED();
    } else if (callType == Call::OTHER_CALL){
        VariableProxy *proxy = callee->AsVariableProxy();
        Visit(proxy);
        auto newClass = PopContext();
        
        ZoneList<Expression*> *args = node->arguments();
        for (int i = 0; i <args->length(); i++) {
            Visit(args->at(i));
        }
        
        std::vector<llvm::Value *> finalArgs;
        for (unsigned i = 0; i < args->length(); i++){
            llvm::Value *arg = PopContext();
            finalArgs.push_back(arg);
        }
        std::reverse(finalArgs.begin(), finalArgs.end());
        llvm::Value *target = _runtime->messageSend(newClass, "cujs_new");
        auto value = _runtime->invokeJSValue(target, finalArgs);
        PushValueToContext(value);
    } else if (callType == Call::PROPERTY_CALL){
        EmitPropertyCall(callee, node->arguments());
    } else {
        UNIMPLEMENTED();
    }
}

// Runtime are only used to support the v8 nature of the AST
// cujs should not extend javascript or rely on v8 specific features
void CGJS::VisitCallRuntime(CallRuntime *node) {
    std::string name = stringFromV8AstRawString(node->raw_name());
    ZoneList<Expression*> *args = node->arguments();

    if (name == "initializeVarGlobal") {
        //create store for this global in the module initializer
        
        //The name is the first arg
        auto globalRawName = (AstRawString *)args->at(0)->AsLiteral()->raw_value()->AsString();
        std::string globalName = stringFromV8AstRawString(globalRawName);
        
        auto moduleInitBB = &_module->getFunction(*_name)->getBasicBlockList().front();
        auto insertPt = _builder->GetInsertBlock();
        
        _builder->SetInsertPoint(moduleInitBB);

        //The value is the 3rd arg
        Visit(args->at(2));
        auto value = PopContext();

        auto global = _module->getGlobalVariable(globalName);
        assert(global && "global var should be initialized");
        
        _builder->CreateStore(value, global);
        _builder->SetInsertPoint(insertPt);
        
        PushValueToContext(_builder->CreateLoad(global));
        return;
    }
    
    UNIMPLEMENTED();
}

void CGJS::VisitUnaryOperation(UnaryOperation *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitCountOperation(CountOperation *node) {
    LhsKind assignType = VARIABLE;

    Property *prop = node->expression()->AsProperty();
    // In case of a property we use the uninitialized expression context
    // of the key to detect a named property.
    if (prop != NULL) {
        assignType =
        (prop->key()->IsPropertyName()) ? NAMED_PROPERTY : KEYED_PROPERTY;
    }
 
    const char *assignOpSelector = node->op() == Token::INC ?
                "cujs_increment" : "cujs_decrement";

  
    llvm::AllocaInst *alloca;
    if (assignType == VARIABLE){
        VariableProxy *proxy =  node->expression()->AsVariableProxy();
        alloca = _context->valueForKey(stringFromV8AstRawString(proxy->raw_name()));
        EmitVariableLoad(proxy);
    } else if (assignType == NAMED_PROPERTY) {
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

void CGJS::VisitBinaryOperation(BinaryOperation *expr) {
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

void CGJS::VisitComma(BinaryOperation *expr) {
    UNIMPLEMENTED();
}

void CGJS::EmitLogicalAnd(BinaryOperation *expr) {
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

void CGJS::EmitLogicalOr(BinaryOperation *expr) {
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

void CGJS::VisitArithmeticExpression(BinaryOperation *expr) {
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
static bool IsClassOfTest(CompareOperation *expr) {
    if (expr->op() != Token::EQ_STRICT) return false;
    CallRuntime *call = expr->left()->AsCallRuntime();
    if (call == NULL) return false;
    Literal *literal = expr->right()->AsLiteral();
    if (literal == NULL) return false;
    if (!literal->value()->IsString()) return false;
    if (!call->name()->IsOneByteEqualTo(STATIC_ASCII_VECTOR("_ClassOf"))) {
        return false;
    }
    assert(call->arguments()->length() == 1);
    return true;
}
 
void CGJS::VisitCompareOperation(CompareOperation *expr) {
    assert(!HasStackOverflow());
    llvm::Value *resultValue = NULL;
    
    // Check for a few fast cases. The AST visiting behavior must be in sync
    // with the full codegen: We don't push both left and right values onto
    // the expression stack when one side is a special-case literal.
    Expression *sub_expr = NULL;
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
    } else {
        UNIMPLEMENTED();
    }
    
    assert(resultValue);
    PushValueToContext(resultValue);
}

void CGJS::VisitThisFunction(ThisFunction *node) {
    UNIMPLEMENTED();
}

void CGJS::VisitStartAccumulation(AstNode *expr, bool extendContext) {
    if (extendContext){
        EnterContext();
    }
    
    Visit(expr);
}

void CGJS::EnterContext(){
    ILOG("enter context");
    CGContext *context;
    if (_context) {
        context = _context->Extend();
    } else {
        context = new CGContext();
    }
    
    _contexts.push_back(context);
    _context = context;   
}

void CGJS::VisitStartAccumulation(AstNode *expr) {
    VisitStartAccumulation(expr, false);
}

void CGJS::EndAccumulation() {
    _contexts.pop_back();
    delete _context;
    auto context = _contexts.back();
    _context = context;
}

void CGJS::PushValueToContext(llvm::Value *value) {
    _context->Push(value);
}

llvm::Value *CGJS::PopContext() {
    return _context->Pop();
}

//SymbolIsClass - this symbol is currently known as a class
bool CGJS::SymbolIsClass(std::string symbol) {
    return _runtime->_classes.count(symbol) || _module->getFunction(symbol);
}

//SymbolIsJSFunction - this symbol is known jsfunction
bool CGJS::SymbolIsJSFunction(std::string symbol) {
    return !_runtime->isBuiltin(symbol);
}

bool CGJS::IsInGlobalScope() {
    auto currName = _builder->GetInsertBlock()->getParent()->getName();
    return *_name == currName;
}
