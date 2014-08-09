//
//  cgobjcsmacrovisitor.cpp
//  objcjs
//
//  Created by Jerry Marino on 8/5/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include "cgobjcjs.h"
#include "cgobjcsmacrovisitor.h"
#include "cgobjcjsclang.h"
#include "cgobjcjsruntime.h"

using namespace v8::internal;
using namespace objcjs;

#define CGObjCJSDEBUG 0
#define ILOG(A, ...) if (CGObjCJSDEBUG){ printf(A,##__VA_ARGS__), printf("\n");}

void MacroImport(CGObjCJS *CG, Call *node){
    ZoneList<Expression*>* args = node->arguments();
    auto argExpr = args->at(0);
    auto importPath = argExpr->AsLiteral()->raw_value()->AsString();
    if (!importPath) {
        return;
    }
   
    std::string filePath = stringFromV8AstRawString(importPath);
    auto clangFile = objcjs::ClangFile(filePath);
    
    //Lookup the module init BB
    llvm::Module *module = CG->_module;
    llvm::IRBuilder<> *builder = CG->_builder;
    
    auto function = module->getFunction(*CG->_name);
    llvm::BasicBlock *moduleInitBB = &function->getBasicBlockList().front();
    
    auto insertPt = builder->GetInsertBlock();
    builder->SetInsertPoint(moduleInitBB);

    //Declare classes as global variables in the init BB
    for (auto it = clangFile._classes.begin(); it != clangFile._classes.end(); ++it){
        objcjs::ObjCClass *newClass = *it;
        auto className = newClass->_name;
        NewLocalStringVar(className, CG->_module);
        
        ILOG("Class %s #methods: %lu", className.c_str(), newClass->_methods.size());
        
        for (auto methodIt = newClass->_methods.begin(); methodIt != newClass->_methods.end(); ++methodIt){
            objcjs::ObjCMethod *method = *methodIt;
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

CGObjCJSMacroVisitor::CGObjCJSMacroVisitor(CGObjCJS *CG, v8::internal::Zone *zone) {
    InitializeAstVisitor(zone);
    _cg = CG;
    _macros["objc_import"] = MacroImport;
}

void CGObjCJSMacroVisitor::VisitDeclarations(ZoneList<Declaration*>* declarations){
    for (int i = 0; i < declarations->length(); i++) {
        Visit(declarations->at(i));
    }
}

void CGObjCJSMacroVisitor::VisitStatements(ZoneList<Statement*>* statements){
    for (int i = 0; i < statements->length(); i++) {
        Visit(statements->at(i));
    }
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

void CGObjCJSMacroVisitor::VisitExpressionStatement(ExpressionStatement* node) {
    Visit(node->expression());
}

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
void CGObjCJSMacroVisitor::VisitFunctionLiteral(v8::internal::FunctionLiteral* node) {
    VisitDeclarations(node->scope()->declarations());
    VisitStatements(node->body());
}

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

        CallMacroFnPtr macro = _macros[name];
        if (macro){
            macro(_cg, node);
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
