//
//  codegen.h
//  objcjs
//
//  Created by Jerry Marino on 7/6/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#ifndef __objcjs__codegen__
#define __objcjs__codegen__

#include <iostream>
#include "src/ast.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <llvm-c/Core.h>

#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace v8::internal;
#define STR(v) std::string(v)

class CGContext {
public:
    Scope *_scope;
    std::vector<llvm::Value *> _context;
    std::map<std::string, llvm::AllocaInst*> _namedValues;
    CGContext(){
    }
    
    CGContext *Extend(){
        auto extended = new CGContext();
        extended->_context = _context;
        extended->_namedValues = _namedValues;
        return extended;
    }
    
    size_t size(){
        return _context.size();
    }
    
    void setValue(std::string key, llvm::AllocaInst *value) {
        _namedValues[key] = value;
    }
   
    llvm::AllocaInst *valueForKey(std::string key) {
        return _namedValues[key];
    }
    
    void Push(llvm::Value *value) {
        if (value) {
            _context.push_back(value);
        } else {
            printf("warning: tried to push null value");
        }   
    }
    
    llvm::Value *Pop(){
        if (!_context.size()) {
            return NULL;
        }
        llvm::Value *value = _context.back();
        _context.pop_back();
        return value;
    };
};


class ObjCCodeGen: public  v8::internal::AstVisitor {
public:
    CGContext *_context;
    
    std::vector<CGContext *> Contexts;
    
    llvm::IRBuilder<> *_builder;
    llvm::Module *_module;
    
    llvm::Type *_pointerTy;

    llvm::BasicBlock *_setRetBB;
    llvm::BasicBlock *_defaultRetBB;
    
    ObjCCodeGen(Zone *zone);

    virtual~ObjCCodeGen(){};

    void dump();
   
    void VisitDeclarations(ZoneList<Declaration*>* declarations){
        if (declarations->length() > 0) {
            for (int i = 0; i < declarations->length(); i++) {
                Visit(declarations->at(i));
            }
        }
    }
    void VisitStatements(ZoneList<Statement*>* statements){
        for (int i = 0; i < statements->length(); i++) {
            Visit(statements->at(i));
        }
    }

    void VisitBlock(Block *block){
        VisitStatements(block->statements());
    }
    
    void VisitVariableDeclaration(v8::internal::VariableDeclaration* node);
    void VisitFunctionDeclaration(v8::internal::FunctionDeclaration* node);
    void VisitModuleDeclaration(ModuleDeclaration* node);
    void VisitImportDeclaration(ImportDeclaration* node);
    void VisitExportDeclaration(ExportDeclaration* node);
    void VisitModuleLiteral(ModuleLiteral* node);
    void VisitModuleVariable(ModuleVariable* node);
    void VisitModulePath(ModulePath* node);
    void VisitModuleUrl(ModuleUrl* node);
    void VisitModuleStatement(ModuleStatement* node);
    void VisitExpressionStatement(ExpressionStatement* node);
    void VisitEmptyStatement(EmptyStatement* node);
    void VisitIfStatement(IfStatement* node);
    void VisitContinueStatement(ContinueStatement* node);
    void VisitBreakStatement(BreakStatement* node);
    void VisitReturnStatement(ReturnStatement* node);
    void VisitWithStatement(WithStatement* node);
    void VisitSwitchStatement(SwitchStatement* node);
    void VisitCaseClause(CaseClause* clause);
    void VisitDoWhileStatement(DoWhileStatement* node);
    void VisitWhileStatement(WhileStatement* node);
    void VisitForStatement(ForStatement* node);
    void VisitForInStatement(ForInStatement* node);
    void VisitForOfStatement(ForOfStatement* node);
    void VisitTryCatchStatement(TryCatchStatement* node);
    void VisitTryFinallyStatement(TryFinallyStatement* node);
    void VisitDebuggerStatement(DebuggerStatement* node);
    void VisitFunctionLiteral(v8::internal::FunctionLiteral* node);
    void VisitNativeFunctionLiteral(NativeFunctionLiteral* node);
    void VisitConditional(Conditional* node);
    void VisitLiteral(Literal* node);
    
    void VisitRegExpLiteral(RegExpLiteral* node) ;
    void VisitObjectLiteral(ObjectLiteral* node) ;
    void VisitArrayLiteral(ArrayLiteral* node) ;
    void VisitVariableProxy(VariableProxy* node);
    void VisitAssignment(Assignment* node) ;
    void VisitYield(Yield* node) ;
    void VisitThrow(Throw* node);
    void VisitProperty(Property* node);
    void VisitCall(Call* node) ;
    void VisitCallNew(CallNew* node);
    void VisitCallRuntime(CallRuntime* node);
    void VisitUnaryOperation(UnaryOperation* node) ;
    void VisitCountOperation(CountOperation* node);
    void VisitBinaryOperation(BinaryOperation* node);
    void VisitCompareOperation(CompareOperation* node);
    void VisitThisFunction(ThisFunction* node) ;

    void VisitComma(BinaryOperation* expr);
    void VisitLogicalExpression(BinaryOperation* expr);
    void VisitArithmeticExpression(BinaryOperation* expr);
    
    void EmitVariableAssignment(Variable* var,
                                               Token::Value op) ;
    
    void EmitVariableLoad(VariableProxy* proxy);

    void EmitBinaryOp(BinaryOperation* expr, Token::Value op);
    
    llvm::Value *CGLiteral( Handle<Object> value, bool push);
    void CGIfStatement(IfStatement *node, bool flag);

    void VisitStartAccumulation(AstNode *expr);
    void VisitStartAccumulation(AstNode *expr, bool extendContext);
    
    void EndAccumulation();
    void VisitStartStackAccumulation(AstNode *expr);
    void EndStackAccumulation();
    void CreateArgumentAllocas(llvm::Function *F, v8::internal::Scope* node);
    void CreateJSArgumentAllocas(llvm::Function *F, v8::internal::Scope* node);
   
#pragma mark - Runtime calls
    llvm::Value *newString(std::string string);
    llvm::Value *newNumber(double value);
    llvm::Value *newNumberWithLLVMValue(llvm::Value *value);
    llvm::Value *doubleValue(llvm::Value *llvmValue);
    llvm::Value *boolValue(llvm::Value *llvmValue);
    llvm::Value *messageSend(llvm::Value *receiver, const char *selector, std::vector<llvm::Value *>ArgsV);
    llvm::Value *messageSend(llvm::Value *receiver, const char *selector, llvm::Value *Arg);
    llvm::Value *messageSend(llvm::Value *receiver, const char *selector);
    llvm::Value *messageSendJSFunction(llvm::Value *instance, std::vector<llvm::Value *>ArgsV);
    llvm::Value *classNamed(const char *name);
    
    void PushValueToContext(llvm::Value *value);
    llvm::Value *PopContext();
    
    DEFINE_AST_VISITOR_SUBCLASS_MEMBERS ();
};


#endif /* defined(__objcjs__codegen__) */
