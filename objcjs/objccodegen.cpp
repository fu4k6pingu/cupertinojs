//
//  codegen.cpp
//  objcjs
//
//  Created by Jerry Marino on 7/6/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include "objccodegen.h"
#include <llvm-c/Core.h>

using namespace v8::internal;

//
//ObjCCodeGen::ObjCCodeGen(Zone* zone) {
////  InitializeAstVisitor(zone);
//}


//ObjCCodeGen::~ObjCCodeGen() {
////  DeleteArray(output_);
//}
//virtual void ObjCCodeGen::VisitDeclarations(v8::internal::ZoneList<Declaration*>* declarations) {
//
//}
//    
//virtual void ObjCCodeGen::VisitStatements(ZoneList<v8::internal::Statement*>* statements) {
//
//}
//    
//void ObjCCodeGen::VisitExpressions(ZoneList<Expression*>* expressions) {
//
//}

//void ObjCCodeGen::VisitBlock(Block* node) {
//  if (!node->is_initializer_block()) Print("{ ");
//  PrintStatements(node->statements());
//  if (node->statements()->length() > 0) Print(" ");
//  if (!node->is_initializer_block()) Print("}");
//    ASSERT(node);
//}


void ObjCCodeGen::VisitVariableDeclaration(v8::internal::VariableDeclaration* node) {
//  Print("var ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(";");
}


void ObjCCodeGen::VisitFunctionDeclaration(v8::internal::FunctionDeclaration* node) {
//  Print("function ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(" = ");
//  PrintFunctionLiteral(node->fun());
//  Print(";");

    
}


void ObjCCodeGen::VisitModuleDeclaration(ModuleDeclaration* node) {
//  Print("module ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(" = ");
//  Visit(node->module());
//  Print(";");
}


void ObjCCodeGen::VisitImportDeclaration(ImportDeclaration* node) {
//  Print("import ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(" from ");
//  Visit(node->module());
//  Print(";");
}


void ObjCCodeGen::VisitExportDeclaration(ExportDeclaration* node) {
//  Print("export ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(";");
}


void ObjCCodeGen::VisitModuleLiteral(ModuleLiteral* node) {
//  VisitBlock(node->body());
}


void ObjCCodeGen::VisitModuleVariable(ModuleVariable* node) {
//  Visit(node->proxy());
}


void ObjCCodeGen::VisitModulePath(ModulePath* node) {
//  Visit(node->module());
//  Print(".");
//  PrintLiteral(node->name(), false);
}


void ObjCCodeGen::VisitModuleUrl(ModuleUrl* node) {
//  Print("at ");
//  PrintLiteral(node->url(), true);
}


void ObjCCodeGen::VisitModuleStatement(ModuleStatement* node) {
//  Print("module ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(" ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitExpressionStatement(ExpressionStatement* node) {
//  Visit(node->expression());
//  Print(";");
}


void ObjCCodeGen::VisitEmptyStatement(EmptyStatement* node) {
//  Print(";");
}


void ObjCCodeGen::VisitIfStatement(IfStatement* node) {
//  Print("if (");
//  Visit(node->condition());
//  Print(") ");
//  Visit(node->then_statement());
//  if (node->HasElseStatement()) {
//    Print(" else ");
//    Visit(node->else_statement());
//  }
}


void ObjCCodeGen::VisitContinueStatement(ContinueStatement* node) {
//  Print("continue");
//  ZoneList<const AstRawString*>* labels = node->target()->labels();
//  if (labels != NULL) {
//    Print(" ");
//    ASSERT(labels->length() > 0);  // guaranteed to have at least one entry
//    PrintLiteral(labels->at(0), false);  // any label from the list is fine
//  }
//  Print(";");
}


void ObjCCodeGen::VisitBreakStatement(BreakStatement* node) {
//  Print("break");
//  ZoneList<const AstRawString*>* labels = node->target()->labels();
//  if (labels != NULL) {
//    Print(" ");
//    ASSERT(labels->length() > 0);  // guaranteed to have at least one entry
//    PrintLiteral(labels->at(0), false);  // any label from the list is fine
//  }
//  Print(";");
}


void ObjCCodeGen::VisitReturnStatement(ReturnStatement* node) {
//  Print("return ");
//  Visit(node->expression());
//  Print(";");
}


void ObjCCodeGen::VisitWithStatement(WithStatement* node) {
//  Print("with (");
//  Visit(node->expression());
//  Print(") ");
//  Visit(node->statement());
}


void ObjCCodeGen::VisitSwitchStatement(SwitchStatement* node) {
//  PrintLabels(node->labels());
//  Print("switch (");
//  Visit(node->tag());
//  Print(") { ");
//  ZoneList<CaseClause*>* cases = node->cases();
//  for (int i = 0; i < cases->length(); i++)
//    Visit(cases->at(i));
//  Print("}");
}


void ObjCCodeGen::VisitCaseClause(CaseClause* clause) {
//  if (clause->is_default()) {
//    Print("default");
//  } else {
//    Print("case ");
//    Visit(clause->label());
//  }
//  Print(": ");
//  PrintStatements(clause->statements());
//  if (clause->statements()->length() > 0)
//    Print(" ");
}


void ObjCCodeGen::VisitDoWhileStatement(DoWhileStatement* node) {
//  PrintLabels(node->labels());
//  Print("do ");
//  Visit(node->body());
//  Print(" while (");
//  Visit(node->cond());
//  Print(");");
}


void ObjCCodeGen::VisitWhileStatement(WhileStatement* node) {
//  PrintLabels(node->labels());
//  Print("while (");
//  Visit(node->cond());
//  Print(") ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitForStatement(ForStatement* node) {
//  PrintLabels(node->labels());
//  Print("for (");
//  if (node->init() != NULL) {
//    Visit(node->init());
//    Print(" ");
//  } else {
//    Print("; ");
//  }
//  if (node->cond() != NULL) Visit(node->cond());
//  Print("; ");
//  if (node->next() != NULL) {
//    Visit(node->next());  // prints extra ';', unfortunately
//    // to fix: should use Expression for next
//  }
//  Print(") ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitForInStatement(ForInStatement* node) {
//  PrintLabels(node->labels());
//  Print("for (");
//  Visit(node->each());
//  Print(" in ");
//  Visit(node->enumerable());
//  Print(") ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitForOfStatement(ForOfStatement* node) {
//  PrintLabels(node->labels());
//  Print("for (");
//  Visit(node->each());
//  Print(" of ");
//  Visit(node->iterable());
//  Print(") ");
//  Visit(node->body());
}


void ObjCCodeGen::VisitTryCatchStatement(TryCatchStatement* node) {
//  Print("try ");
//  Visit(node->try_block());
//  Print(" catch (");
//  const bool quote = false;
//  PrintLiteral(node->variable()->name(), quote);
//  Print(") ");
//  Visit(node->catch_block());
}


void ObjCCodeGen::VisitTryFinallyStatement(TryFinallyStatement* node) {
//  Print("try ");
//  Visit(node->try_block());
//  Print(" finally ");
//  Visit(node->finally_block());
}


void ObjCCodeGen::VisitDebuggerStatement(DebuggerStatement* node) {
//  Print("debugger ");
}


void ObjCCodeGen::VisitFunctionLiteral(v8::internal::FunctionLiteral* node) {
//  Print("(");
//  PrintFunctionLiteral(node);
//  Print(")");
}


void ObjCCodeGen::VisitNativeFunctionLiteral(NativeFunctionLiteral* node) {
//  Print("(");
//  PrintLiteral(node->name(), false);
//  Print(")");
}


void ObjCCodeGen::VisitConditional(Conditional* node) {
//  Visit(node->condition());
//  Print(" ? ");
//  Visit(node->then_expression());
//  Print(" : ");
//  Visit(node->else_expression());
}


void ObjCCodeGen::VisitLiteral(Literal* node) {
//  PrintLiteral(node->value(), true);
}


void ObjCCodeGen::VisitRegExpLiteral(RegExpLiteral* node) {
//  Print(" RegExp(");
//  PrintLiteral(node->pattern(), false);
//  Print(",");
//  PrintLiteral(node->flags(), false);
//  Print(") ");
}


void ObjCCodeGen::VisitObjectLiteral(ObjectLiteral* node) {
//  Print("{ ");
//  for (int i = 0; i < node->properties()->length(); i++) {
//    if (i != 0) Print(",");
//    ObjectLiteral::Property* property = node->properties()->at(i);
//    Print(" ");
//    Visit(property->key());
//    Print(": ");
//    Visit(property->value());
//  }
//  Print(" }");
}


void ObjCCodeGen::VisitArrayLiteral(ArrayLiteral* node) {
//  Print("[ ");
//  for (int i = 0; i < node->values()->length(); i++) {
//    if (i != 0) Print(",");
//    Visit(node->values()->at(i));
//  }
//  Print(" ]");
}


void ObjCCodeGen::VisitVariableProxy(VariableProxy* node) {
//  PrintLiteral(node->name(), false);
}


void ObjCCodeGen::VisitAssignment(Assignment* node) {
//  Visit(node->target());
//  Print(" %s ", Token::String(node->op()));
//  Visit(node->value());
}


void ObjCCodeGen::VisitYield(Yield* node) {
//  Print("yield ");
//  Visit(node->expression());
}


void ObjCCodeGen::VisitThrow(Throw* node) {
//  Print("throw ");
//  Visit(node->exception());
}


void ObjCCodeGen::VisitProperty(Property* node) {
//  Expression* key = node->key();
//  Literal* literal = key->AsLiteral();
//  if (literal != NULL && literal->value()->IsInternalizedString()) {
//    Print("(");
//    Visit(node->obj());
//    Print(").");
//    PrintLiteral(literal->value(), false);
//  } else {
//    Visit(node->obj());
//    Print("[");
//    Visit(key);
//    Print("]");
//  }
}


void ObjCCodeGen::VisitCall(Call* node) {
//  Visit(node->expression());
//  PrintArguments(node->arguments());
}


void ObjCCodeGen::VisitCallNew(CallNew* node) {
//  Print("new (");
//  Visit(node->expression());
//  Print(")");
//  PrintArguments(node->arguments());
}


void ObjCCodeGen::VisitCallRuntime(CallRuntime* node) {
//  Print("%%");
//  PrintLiteral(node->name(), false);
//  PrintArguments(node->arguments());
}


void ObjCCodeGen::VisitUnaryOperation(UnaryOperation* node) {
//  Token::Value op = node->op();
//  bool needsSpace =
//      op == Token::DELETE || op == Token::TYPEOF || op == Token::VOID;
//  Print("(%s%s", Token::String(op), needsSpace ? " " : "");
//  Visit(node->expression());
//  Print(")");
}


void ObjCCodeGen::VisitCountOperation(CountOperation* node) {
//  Print("(");
//  if (node->is_prefix()) Print("%s", Token::String(node->op()));
//  Visit(node->expression());
//  if (node->is_postfix()) Print("%s", Token::String(node->op()));
//  Print(")");
}


void ObjCCodeGen::VisitBinaryOperation(BinaryOperation* node) {
//  Print("(");
//  Visit(node->left());
//  Print(" %s ", Token::String(node->op()));
//  Visit(node->right());
//  Print(")");
}


void ObjCCodeGen::VisitCompareOperation(CompareOperation* node) {
//  Print("(");
//  Visit(node->left());
//  Print(" %s ", Token::String(node->op()));
//  Visit(node->right());
//  Print(")");
}


void ObjCCodeGen::VisitThisFunction(ThisFunction* node) {
//  Print("<this-function>");
}
