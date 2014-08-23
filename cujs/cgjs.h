//
//  codegen.h
//  cujs
//
//  Created by Jerry Marino on 7/6/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#ifndef __cujs__codegen__
#define __cujs__codegen__

#include <iostream>
#include <cctype>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <src/ast.h>
#include <src/arguments.h>
#include <src/parser.h>
#include <src/prettyprinter.h>
#include <include/libplatform/libplatform.h>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm-c/Core.h>

using namespace v8::internal;

namespace cujs{
    std::string stringFromV8AstRawString(const AstRawString *raw);
   
    class CGContext;
    class CGJSRuntime;
    class CGJSMacroVisitor;
   
    class CGJS: public  v8::internal::AstVisitor {
        CompilationInfoWithZone *_info;

    public:
        std::string *_name;
        CGContext *_context;
        std::vector<CGContext *> _contexts;
        llvm::IRBuilder<> *_builder;
        llvm::Module *_module;
        
        typedef void(*CallMacroFnPtr)(CGJS *CG, Call *node);
        std::map <std::string, CallMacroFnPtr> _macros;
        
        llvm::BasicBlock *_currentSetRetBlock;
        std::map <Token::Value, std::string> assignOpSelectorByToken;
        std::map <Token::Value, std::string> opSelectorByToken;
        std::map <int, std::string> _nameByFunctionID;


        std::map <llvm::Function *, llvm::BasicBlock *> returnBlockByFunction;
        
        CGJSRuntime *_runtime;
        void *_macroVisitor;
        
        CGJS(std::string name,
                 CompilationInfoWithZone *info);
        
        ~CGJS();
        
        void Codegen();
        void Dump();
        
        void VisitDeclarations(ZoneList<Declaration*>* declarations);
        void VisitStatements(ZoneList<Statement*>* statements);
        void VisitBlock(Block *block);
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
        void VisitSuperReference(SuperReference* node);
        void VisitComma(BinaryOperation* expr);
        void VisitLogicalExpression(BinaryOperation* expr);
        void VisitArithmeticExpression(BinaryOperation* expr);
        
        void EmitVariableAssignment(Variable* var,
                                    Token::Value op) ;
        
        void EmitFunctionPrototype(v8::internal::FunctionLiteral* node);
        void EmitNamedPropertyAssignment(Property *property, llvm::Value *value);
        void EmitKeyedPropertyAssignment(llvm::Value *target, llvm::Value *key, llvm::Value *value);
        void EmitPropertyCall(Expression *expr, ZoneList<Expression*>* args);
        
        void EmitVariableLoad(VariableProxy* proxy);
        void EmitVariableStore(VariableProxy* proxy, llvm::Value *value);
        void EmitBinaryOp(BinaryOperation* expr, Token::Value op);
        void EmitLogicalAnd(BinaryOperation *expr);
        void EmitLogicalOr(BinaryOperation *expr);
        
        void EmitStructLoadCall(Call* node);
        void EmitStructLoadCast(std::string structName, llvm::CallInst *value);
        
        llvm::Value *CGLiteral( Handle<Object> value, bool push);
        void CGIfStatement(IfStatement *node, bool flag);
        
        void EnterContext();
        void VisitStartAccumulation(AstNode *expr);
        void VisitStartAccumulation(AstNode *expr, bool extendContext);
        
        void EndAccumulation();
        void VisitStartStackAccumulation(AstNode *expr);
        void EndStackAccumulation();
        void CreateArgumentAllocas(llvm::Function *F,
                                   v8::internal::Scope* node);
        void CreateJSArgumentAllocas(llvm::Function *F,
                                     v8::internal::Scope* node);
        
        void PushValueToContext(llvm::Value *value);
        llvm::Value *PopContext();
        
        bool SymbolIsClass(std::string symbol);
        bool SymbolIsJSFunction(std::string symbol);
        bool IsInGlobalScope();
        
        std::vector <llvm::Value *>makeArgs(ZoneList<Expression*>* args);
        
        friend CGJSMacroVisitor;
        
        DEFINE_AST_VISITOR_SUBCLASS_MEMBERS ();
    };
};

#endif /* defined(__cujs__codegen__) */
