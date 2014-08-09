//
//  cgobjcsmacrovisitor.cpp
//  cujs
//
//  Created by Jerry Marino on 8/5/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include "cgjs.h"
#include "cgjsmacrovisitor.h"
#include "cgclang.h"
#include "cgjsruntime.h"

using namespace v8::internal;
using namespace cujs;

#define CGJSDEBUG 0
#define ILOG(A, ...) if (CGJSDEBUG){ printf(A,##__VA_ARGS__), printf("\n");}

void MacroImport(CGJS *CG, Call *node){
    ZoneList<Expression*>* args = node->arguments();
    auto argExpr = args->at(0);
    auto importPath = argExpr->AsLiteral()->raw_value()->AsString();
    if (!importPath) {
        return;
    }
   
    std::string filePath = stringFromV8AstRawString(importPath);
    auto clangFile = cujs::ClangFile(filePath);
    
    //Lookup the module init BB
    llvm::Module *module = CG->_module;
    llvm::IRBuilder<> *builder = CG->_builder;
    
    auto function = module->getFunction(*CG->_name);
    llvm::BasicBlock *moduleInitBB = &function->getBasicBlockList().front();
    
    auto insertPt = builder->GetInsertBlock();
    builder->SetInsertPoint(moduleInitBB);

    //Declare classes as global variables in the init BB
    for (auto it = clangFile._classes.begin(); it != clangFile._classes.end(); ++it){
        cujs::ObjCClass *newClass = *it;
        auto className = newClass->_name;
        NewLocalStringVar(className, CG->_module);
        
        ILOG("Class %s #methods: %lu", className.c_str(), newClass->_methods.size());
        
        for (auto methodIt = newClass->_methods.begin(); methodIt != newClass->_methods.end(); ++methodIt){
            cujs::ObjCMethod *method = *methodIt;
            ILOG("Method (%d) %s: %lu ", method->type, method->name.c_str(), method->params.size());

            std::string objCSelector = method->name;
            auto jsSelector = ObjCSelectorToJS(objCSelector);
            
            ObjCMethod *existingMethod = ((ObjCMethod *)CG->_objCMethodBySelector[jsSelector]);
            if(!existingMethod){
                CG->_objCMethodBySelector[jsSelector] = method;
                ILOG("JS selector name %s", jsSelector.c_str());
            }
        }
       
        auto global = module->getGlobalVariable(className);
        if (!global) {
            llvm::Value *globalValue = CG->_runtime->declareGlobal(className);
            builder->CreateStore(CG->_runtime->classNamed(className.c_str()), globalValue);
            CG->_classes.insert(className);
        }
    }
    
    //Restore insert point
    builder->SetInsertPoint(insertPt);
}

CGJSMacroVisitor::CGJSMacroVisitor(CGJS *CG, v8::internal::Zone *zone) {
    InitializeAstVisitor(zone);
    _cg = CG;
    _macros["objc_import"] = MacroImport;
}

void CGJSMacroVisitor::VisitDeclarations(ZoneList<Declaration*>* declarations){
    for (int i = 0; i < declarations->length(); i++) {
        Visit(declarations->at(i));
    }
}

void CGJSMacroVisitor::VisitStatements(ZoneList<Statement*>* statements){
    for (int i = 0; i < statements->length(); i++) {
        Visit(statements->at(i));
    }
}

void CGJSMacroVisitor::VisitBlock(Block *block){
    VisitStatements(block->statements());
}

void CGJSMacroVisitor::VisitVariableDeclaration(v8::internal::VariableDeclaration* node) {
}

void CGJSMacroVisitor::VisitFunctionDeclaration(v8::internal::FunctionDeclaration* node) {
}

#pragma mark - Modules

void CGJSMacroVisitor::VisitModuleDeclaration(ModuleDeclaration* node) {}

void CGJSMacroVisitor::VisitImportDeclaration(ImportDeclaration* node) {}

void CGJSMacroVisitor::VisitExportDeclaration(ExportDeclaration* node) {}

void CGJSMacroVisitor::VisitModuleLiteral(ModuleLiteral* node) {}

void CGJSMacroVisitor::VisitModuleVariable(ModuleVariable* node) {}

void CGJSMacroVisitor::VisitModulePath(ModulePath* node) {}

void CGJSMacroVisitor::VisitModuleUrl(ModuleUrl* node) {}

void CGJSMacroVisitor::VisitModuleStatement(ModuleStatement* node) {}

#pragma mark - Statements

void CGJSMacroVisitor::VisitExpressionStatement(ExpressionStatement* node) {
    Visit(node->expression());
}

void CGJSMacroVisitor::VisitEmptyStatement(EmptyStatement* node) {}

void CGJSMacroVisitor::VisitIfStatement(IfStatement* node) {}

void CGJSMacroVisitor::VisitContinueStatement(ContinueStatement* node) {}

void CGJSMacroVisitor::VisitBreakStatement(BreakStatement* node) {}

void CGJSMacroVisitor::VisitReturnStatement(ReturnStatement* node) {}

void CGJSMacroVisitor::VisitWithStatement(WithStatement* node) {}

#pragma mark - Switch

void CGJSMacroVisitor::VisitSwitchStatement(SwitchStatement* node) {
    UNIMPLEMENTED();
}

void CGJSMacroVisitor::VisitCaseClause(CaseClause* clause) {}

#pragma mark - Loops

void CGJSMacroVisitor::VisitWhileStatement(WhileStatement* node){}

void CGJSMacroVisitor::VisitDoWhileStatement(DoWhileStatement* node) {}

void CGJSMacroVisitor::VisitForStatement(ForStatement* node) {}

void CGJSMacroVisitor::VisitForInStatement(ForInStatement* node) {}

void CGJSMacroVisitor::VisitForOfStatement(ForOfStatement* node) {
    UNIMPLEMENTED();
}

#pragma mark - Try
void CGJSMacroVisitor::VisitTryCatchStatement(TryCatchStatement* node) {}

void CGJSMacroVisitor::VisitTryFinallyStatement(TryFinallyStatement* node) {}

void CGJSMacroVisitor::VisitDebuggerStatement(DebuggerStatement* node) {}

//Define the body of the function
void CGJSMacroVisitor::VisitFunctionLiteral(v8::internal::FunctionLiteral* node) {
    VisitDeclarations(node->scope()->declarations());
    VisitStatements(node->body());
}

void CGJSMacroVisitor::VisitNativeFunctionLiteral(NativeFunctionLiteral* node) {}

void CGJSMacroVisitor::VisitConditional(Conditional* node) {}

void CGJSMacroVisitor::VisitLiteral(class Literal* node) {}

void CGJSMacroVisitor::VisitRegExpLiteral(RegExpLiteral* node) {}

void CGJSMacroVisitor::VisitObjectLiteral(ObjectLiteral* node){}

void CGJSMacroVisitor::VisitArrayLiteral(ArrayLiteral* node) {}

void CGJSMacroVisitor::VisitVariableProxy(VariableProxy* node) {}

void CGJSMacroVisitor::VisitAssignment(Assignment* node) {}

void CGJSMacroVisitor::VisitYield(Yield* node) {}

void CGJSMacroVisitor::VisitThrow(Throw* node) {}

void CGJSMacroVisitor::VisitProperty(Property* node) {}

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

void CGJSMacroVisitor::VisitCall(Call* node) {
    Expression *callee = node->expression();
    Call::CallType callType = GetCallType(callee, isolate());
    if (callType == Call::OTHER_CALL){
        VariableProxy* proxy = callee->AsVariableProxy();
        std::string name = stringFromV8AstRawString(proxy->raw_name());

        CallMacroFnPtr macro = _macros[name];
        if (macro){
            macro(_cg, node);
        }
    }
}

void CGJSMacroVisitor::VisitCallNew(CallNew* node) {}

void CGJSMacroVisitor::VisitCallRuntime(CallRuntime* node) {}

void CGJSMacroVisitor::VisitUnaryOperation(UnaryOperation* node) {}

void CGJSMacroVisitor::VisitCountOperation(CountOperation* node) {}

void CGJSMacroVisitor::VisitBinaryOperation(BinaryOperation* expr) {}

void CGJSMacroVisitor::VisitComma(BinaryOperation* expr) {}

void CGJSMacroVisitor::VisitArithmeticExpression(BinaryOperation* expr) {}

void CGJSMacroVisitor::VisitCompareOperation(CompareOperation* expr) {}

void CGJSMacroVisitor::VisitThisFunction(ThisFunction* node) {}
