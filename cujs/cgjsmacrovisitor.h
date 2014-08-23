//
//  cgobjcsmacrovisitor.h
//  cujs
//
//  Created by Jerry Marino on 8/5/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#ifndef __cujs__cgobjcsmacrovisitor__
#define __cujs__cgobjcsmacrovisitor__

namespace cujs {
    class CGJSMacroVisitor: public  v8::internal::AstVisitor {
        CGJS *_cg;
    public:
        
        // Macros are references to native functions
        // which override javascript calls
        typedef void(*CallMacroFnPtr)(CGJS *CG, Call *node);
        std::map <std::string, CallMacroFnPtr> _macros;
        
        CGJSMacroVisitor(CGJS *CG, v8::internal::Zone *zone);
        virtual~CGJSMacroVisitor(){};
        
        void dump();
        
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
        
        DEFINE_AST_VISITOR_SUBCLASS_MEMBERS ();
    };
}

#endif /* defined(__cujs__cgobjcsmacrovisitor__) */
