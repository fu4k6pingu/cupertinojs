//
//  codegen.cpp
//  objcjs
//
//  Created by Jerry Marino on 7/6/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include "objccodegen.h"
#include <llvm-c/Core.h>
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "src/scopes.h"

using namespace v8::internal;

static std::string stringFromV8AstRawString(const AstRawString *raw){
    std::string str;
    size_t size = raw->length();
    const unsigned char *data = raw->raw_data();
    for (int i = 0; i < size; i++) {
        str += data[i];
    }
    return str;
}
/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction,
                                          const std::string &VarName) {
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                     TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(llvm::Type::getDoubleTy(llvm::getGlobalContext()), 0,
                             VarName.c_str());
}

void ObjCCodeGen::dump(){
    _module->dump();
}

//
//ObjCCodeGen::ObjCCodeGen(Zone* zone) {
////  InitializeAstVisitor(zone);
//}

//void ObjCCodeGen::Visit(v8::internal::AstNode *node){
//    
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


//Value *CallExprAST() {
//    // Look up the name in the global module table.
//;
//}

void ObjCCodeGen::VisitVariableDeclaration(v8::internal::VariableDeclaration* node) {
//  Print("var ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(";");
    
    //TODO : Enter into symbol table with scope..
    std::string str = stringFromV8AstRawString(node->proxy()->raw_name());
    llvm::Function *f = _builder->GetInsertBlock()->getParent();
    _namedValues[str] = CreateEntryBlockAlloca(f, str);
    CGLiteral(node->proxy()->name());
}


void ObjCCodeGen::VisitFunctionDeclaration(v8::internal::FunctionDeclaration* node) {
//  Print("function ");
//  PrintLiteral(node->proxy()->name(), false);
//  Print(" = ");
//  PrintFunctionLiteral(node->fun());
//  Print(";");

  
    //TODO : this needs to assign the function
//    VisitLiteral(node->proxy()->name());

   
    VisitFunctionLiteral(node->fun());
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
  Visit(node->expression());
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
    Visit(node->expression());

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
    v8::internal::Scope *scope = node->scope();
    int num_params = scope->num_parameters();

    // Make the function type:  double(double,double) etc.
    std::vector<llvm::Type*> Doubles(num_params,
                               llvm::Type::getDoubleTy(llvm::getGlobalContext()));
    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(llvm::getGlobalContext()),
                                         Doubles, false);


    std::string str = stringFromV8AstRawString(node->raw_name());
    llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, str, _module);
 
  
    _currentFunction = F;
    // Create a new basic block to start insertion into.
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", F);
    _builder->SetInsertPoint(BB);
    
    VisitDeclarations(node->scope()->declarations());
    VisitStatements(node->body());
    
    if (_retValue) {
        _builder->CreateRet(_retValue);
        _retValue = NULL;
    } else {
        _builder->CreateRetVoid();
    }

    _builder->saveAndClearIP();
    _currentFunction = NULL;
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


void ObjCCodeGen::VisitLiteral(class Literal* node) {
    CGLiteral(node->value());
}

llvm::Value *ObjCCodeGen::CGLiteral( Handle<Object> value) {
  Object* object = *value;
    llvm::Value *lvalue;
  if (object->IsString()) {
    String* string = String::cast(object);
//    if (quote) printf("\"");
    for (int i = 0; i < string->length(); i++) {
//      printf("%c", string->Get(i));
    }
//    if (quote) printf("\"");
  } else if (object->IsNull()) {
    printf("null");
  } else if (object->IsTrue()) {
    printf("true");
  } else if (object->IsFalse()) {
//    printf("false");
  } else if (object->IsUndefined()) {
    printf("undefined");
  } else if (object->IsNumber()) {
    printf("%g", object->Number());
      lvalue = llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(object->Number()));
  } else if (object->IsJSObject()) {
    // regular expression
    if (object->IsJSFunction()) {
//      printf("JS-Function");
    } else if (object->IsJSArray()) {
//      printf("JS-array[%u]", JSArray::cast(object)->length());
    } else if (object->IsJSObject()) {
      printf("JS-Object");
    } else {
      printf("?UNKNOWN?");
    }
  } else if (object->IsFixedArray()) {
    printf("FixedArray");
  } else {
    printf("<unknown literal %p>", object);
  }
 
    if (_accumulatorContext) {
        _accumulatorContext->push_back(lvalue);
    }
    
    _retValue = lvalue;
    return NULL;
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
    std::string str = stringFromV8AstRawString(node->raw_name());
    
    llvm::Value *var = _namedValues[str];
    if (!var) {
        assert(0 == "Unknown variable name");
    }
    
    llvm::Value *load = _builder->CreateLoad(var, str);
    if (_accumulatorContext) {
        _accumulatorContext->push_back(load);
    }
}

//TODO : checkout full-codegen-x86.cc
void ObjCCodeGen::VisitAssignment(Assignment* node) {
  Visit(node->target());
//  Print(" %s ", Token::String(node->op()));

  Visit(node->value());
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

Call::CallType GetCallType(Call*call, Isolate* isolate) {
    VariableProxy* proxy = call->expression()->AsVariableProxy();
    if (proxy != NULL && proxy->var()) {
        
        if (proxy->var()->IsUnallocated()) {
            return Call::GLOBAL_CALL;
        } else if (proxy->var()->IsLookupSlot()) {
            return Call::LOOKUP_SLOT_CALL;
        }
    }
    
    Property* property = call->expression()->AsProperty();
    return property != NULL ? Call::PROPERTY_CALL : Call::OTHER_CALL;
}

void ObjCCodeGen::VisitCall(Call* node) {
//  Visit(node->expression());
//  PrintArguments(node->arguments());
//   node->proxy()->raw_name())
   
    Expression *callee = node->expression();
    Call::CallType call_type = GetCallType(node, isolate());
    std::string name;
    if (call_type == Call::GLOBAL_CALL) {
        assert(0 == "unimplemented");
    } else if (call_type == Call::LOOKUP_SLOT_CALL) {
        VariableProxy* proxy = callee->AsVariableProxy();
         name = stringFromV8AstRawString(proxy->raw_name());
    } else {
        VariableProxy* proxy = callee->AsVariableProxy();
        name = stringFromV8AstRawString(proxy->raw_name());
    }
   
    llvm::Function *calleeF = _module->getFunction(name);
    
    if (calleeF == 0){
        printf("Unknown function referenced");
        _retValue = NULL;
        return;
    }

// If argument mismatch error.
//    if (CalleeF->arg_size() != Args.size()) {
//
//        return ErrorV("Incorrect # arguments passed");
//
//    }
    ZoneList<Expression*>* args = node->arguments();
    ASSERT(args->length() == 1);
    VisitStartAccumulation(args->at(0));
    std::vector<llvm::Value*> argsV = *_accumulatorContext;
//    if (argsV.back() == 0) return;
    _retValue = _builder->CreateCall(calleeF, argsV, "calltmp");
    EndAccumulation();
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


void ObjCCodeGen::VisitStartAccumulation(Expression *expr) {
    //Start accumulation
    _accumulatorContext = new std::vector<llvm::Value *>;
    Visit(expr);
   
    //End accumulation
}

void ObjCCodeGen::EndAccumulation(){
    delete _accumulatorContext;
    _accumulatorContext = NULL;
}
