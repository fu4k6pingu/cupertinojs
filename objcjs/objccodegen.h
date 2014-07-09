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

class ObjCCodeGen: public  v8::internal::AstVisitor {
public:
    llvm::IRBuilder<> *_builder;
    
    llvm::Module *_module;
    std::map<std::string, llvm::AllocaInst*> _namedValues;

    llvm::Function *_currentFunction;
    
    ObjCCodeGen(Zone *zone){
        InitializeAstVisitor(zone);
        llvm::LLVMContext &Context = llvm::getGlobalContext();
        _builder = new llvm::IRBuilder<> (Context);
        _module = new llvm::Module("jit", Context);
//        _namedValues = new std::map<std::string, bool>;
    }
    

    //TODO : clean this shit up!
    virtual~ObjCCodeGen(){
        
    }
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
    
    llvm::Value *CGLiteral( Handle<Object> value);

    DEFINE_AST_VISITOR_SUBCLASS_MEMBERS ();
};


#endif /* defined(__objcjs__codegen__) */
