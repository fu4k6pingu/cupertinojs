//
//  cgobjcsmacrovisitor.cpp
//  objcjs
//
//  Created by Jerry Marino on 8/5/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include "cgobjcsmacrovisitor.h"


using namespace v8::internal;
using namespace objcjs;

void CGObjCJSMacroVisitor::VisitDeclarations(ZoneList<Declaration*>* declarations){
//    if (declarations->length() > 0) {
//        auto preBB = _builder->GetInsertBlock();
//        for (int i = 0; i < declarations->length(); i++) {
//            auto BB = _builder->GetInsertBlock();
//            Visit(declarations->at(i));
//            if (BB){
//                _builder->SetInsertPoint(BB);
//            } else {
//                if (preBB)
//                    _builder->SetInsertPoint(preBB);
//            }
//        }
//    }
}

void CGObjCJSMacroVisitor::VisitStatements(ZoneList<Statement*>* statements){
//    auto preBB = _builder->GetInsertBlock();
//    for (int i = 0; i < statements->length(); i++) {
//        auto BB = _builder->GetInsertBlock();
//        Visit(statements->at(i));
//        if (BB){
//            _builder->SetInsertPoint(BB);
//        } else {
//            if (preBB)
//                _builder->SetInsertPoint(preBB);
//        }
//    }
}

void CGObjCJSMacroVisitor::VisitBlock(Block *block){
    VisitStatements(block->statements());
}

void CGObjCJSMacroVisitor::VisitVariableDeclaration(v8::internal::VariableDeclaration* node) {
}

void CGObjCJSMacroVisitor::VisitFunctionDeclaration(v8::internal::FunctionDeclaration* node) {
}

#pragma mark - Modules

void CGObjCJSMacroVisitor::VisitModuleDeclaration(ModuleDeclaration* node) {}

void CGObjCJSMacroVisitor::VisitImportDeclaration(ImportDeclaration* node) {}

void CGObjCJSMacroVisitor::VisitExportDeclaration(ExportDeclaration* node) {}

void CGObjCJSMacroVisitor::VisitModuleLiteral(ModuleLiteral* node) {}

void CGObjCJSMacroVisitor::VisitModuleVariable(ModuleVariable* node) {}

void CGObjCJSMacroVisitor::VisitModulePath(ModulePath* node) {}

void CGObjCJSMacroVisitor::VisitModuleUrl(ModuleUrl* node) {}

void CGObjCJSMacroVisitor::VisitModuleStatement(ModuleStatement* node) {}

#pragma mark - Statements

void CGObjCJSMacroVisitor::VisitExpressionStatement(ExpressionStatement* node) {}

void CGObjCJSMacroVisitor::VisitEmptyStatement(EmptyStatement* node) {}

void CGObjCJSMacroVisitor::VisitIfStatement(IfStatement* node) {}

void CGObjCJSMacroVisitor::VisitContinueStatement(ContinueStatement* node) {}

void CGObjCJSMacroVisitor::VisitBreakStatement(BreakStatement* node) {}

void CGObjCJSMacroVisitor::VisitReturnStatement(ReturnStatement* node) {}

void CGObjCJSMacroVisitor::VisitWithStatement(WithStatement* node) {}

#pragma mark - Switch

void CGObjCJSMacroVisitor::VisitSwitchStatement(SwitchStatement* node) {
    UNIMPLEMENTED();
}

void CGObjCJSMacroVisitor::VisitCaseClause(CaseClause* clause) {}

#pragma mark - Loops

void CGObjCJSMacroVisitor::VisitWhileStatement(WhileStatement* node){}

void CGObjCJSMacroVisitor::VisitDoWhileStatement(DoWhileStatement* node) {}

void CGObjCJSMacroVisitor::VisitForStatement(ForStatement* node) {}

void CGObjCJSMacroVisitor::VisitForInStatement(ForInStatement* node) {}

void CGObjCJSMacroVisitor::VisitForOfStatement(ForOfStatement* node) {
    UNIMPLEMENTED();
}

#pragma mark - Try
void CGObjCJSMacroVisitor::VisitTryCatchStatement(TryCatchStatement* node) {}

void CGObjCJSMacroVisitor::VisitTryFinallyStatement(TryFinallyStatement* node) {}

void CGObjCJSMacroVisitor::VisitDebuggerStatement(DebuggerStatement* node) {}

//Define the body of the function
void CGObjCJSMacroVisitor::VisitFunctionLiteral(v8::internal::FunctionLiteral* node) {}

void CGObjCJSMacroVisitor::VisitNativeFunctionLiteral(NativeFunctionLiteral* node) {}

void CGObjCJSMacroVisitor::VisitConditional(Conditional* node) {}

void CGObjCJSMacroVisitor::VisitLiteral(class Literal* node) {}

void CGObjCJSMacroVisitor::VisitRegExpLiteral(RegExpLiteral* node) {}

void CGObjCJSMacroVisitor::VisitObjectLiteral(ObjectLiteral* node){}

void CGObjCJSMacroVisitor::VisitArrayLiteral(ArrayLiteral* node) {}

void CGObjCJSMacroVisitor::VisitVariableProxy(VariableProxy* node) {}

void CGObjCJSMacroVisitor::VisitAssignment(Assignment* node) {}

void CGObjCJSMacroVisitor::VisitYield(Yield* node) {}

void CGObjCJSMacroVisitor::VisitThrow(Throw* node) {}

void CGObjCJSMacroVisitor::VisitProperty(Property* node) {}

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

void CGObjCJSMacroVisitor::VisitCall(Call* node) {
    Expression *callee = node->expression();
    Call::CallType callType = GetCallType(callee, isolate());
    
    if (callType == Call::OTHER_CALL){
        VariableProxy* proxy = callee->AsVariableProxy();
        std::string name = stringFromV8AstRawString(proxy->raw_name());
        
        //check for macro use and abort
        CallMacroFnPtr macro = _macros[name];
        if (macro){
            macro(this, node);
            return;
        }
    }
}

void CGObjCJSMacroVisitor::VisitCallNew(CallNew* node) {}

void CGObjCJSMacroVisitor::VisitCallRuntime(CallRuntime* node) {}

void CGObjCJSMacroVisitor::VisitUnaryOperation(UnaryOperation* node) {}

void CGObjCJSMacroVisitor::VisitCountOperation(CountOperation* node) {}

void CGObjCJSMacroVisitor::VisitBinaryOperation(BinaryOperation* expr) {}

void CGObjCJSMacroVisitor::VisitComma(BinaryOperation* expr) {}

void CGObjCJSMacroVisitor::VisitArithmeticExpression(BinaryOperation* expr) {}

void CGObjCJSMacroVisitor::VisitCompareOperation(CompareOperation* expr) {}

void CGObjCJSMacroVisitor::VisitThisFunction(ThisFunction* node) {}
