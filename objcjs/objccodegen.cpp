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

/// CreateArgumentAllocas - Create an alloca for each argument and register the
/// argument in the symbol table so that references to it will succeed.
void ObjCCodeGen::CreateArgumentAllocas(llvm::Function *F, v8::internal::Scope* scope) {
    llvm::Function::arg_iterator AI = F->arg_begin();
    int num_params = scope->num_parameters();
    for (unsigned Idx = 0, e = num_params; Idx != e; ++Idx, ++AI) {
        // Create an alloca for this variable.
        Variable *param = scope->parameter(Idx);
        std::string str = stringFromV8AstRawString(param->raw_name());
        llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(F, str);
        
        // Store the initial value into the alloca.
        _builder->CreateStore(AI, Alloca);
        
        // Add arguments to variable symbol table.
        _namedValues[str] = Alloca;
    }
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
    llvm::AllocaInst *alloca = CreateEntryBlockAlloca(f, str);
    _namedValues[str] = alloca;

    //TODO : this should declare the variable to whatever was the cg literal
    CGLiteral(node->proxy()->name());
}


void ObjCCodeGen::VisitFunctionDeclaration(v8::internal::FunctionDeclaration* node) {
//TODO : this needs to assign the function
//    VisitLiteral(node->proxy()->name());

    std::string name = stringFromV8AstRawString(node->fun()->raw_name());
    llvm::Function *calleeF = _module->getFunction(name);
    assert(!calleeF);
    
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
    //TODO : look at visit for effect
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
    _shouldReturn = true;
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
    
    if (num_params){
        CreateArgumentAllocas(F, node->scope());
    }
    
    VisitDeclarations(node->scope()->declarations());
    VisitStatements(node->body());
   
    if (_shouldReturn) {
        if (_context->size()) {
            auto retValue = PopContext();
            _builder->CreateRet(retValue);
        } else {
            _builder->CreateRetVoid();
        }
    }
    
    _shouldReturn = false;

    _builder->saveAndClearIP();
    _currentFunction = NULL;
}

void ObjCCodeGen::VisitNativeFunctionLiteral(NativeFunctionLiteral* node) {
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
    llvm::Value *lvalue = NULL;
    if (object->IsString()) {
        String* string = String::cast(object);
//    if (quote) printf("\"");
//    for (int i = 0; i < string->length(); i++) {
//      printf("%c", string->Get(i));
//    }
//    if (quote) printf("\"");
      printf("str");
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
    PushValueToContext(lvalue);
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
    EmitVariableLoad(node);
}

//TODO : checkout full-codegen-x86.cc
void ObjCCodeGen::VisitAssignment(Assignment* node) {
    ASSERT(node->target()->IsValidReferenceExpression());

    enum LhsKind { VARIABLE, NAMED_PROPERTY, KEYED_PROPERTY };
    LhsKind assign_type = VARIABLE;
    Property* property = node->target()->AsProperty();
    
    if (property != NULL) {
        assign_type = (property->key()->IsPropertyName())
        ? NAMED_PROPERTY
        : KEYED_PROPERTY;
    }
    switch (assign_type) {
        case VARIABLE:{
            //Do nothing
            break;
        }
        case KEYED_PROPERTY:{
            
        }
        case NAMED_PROPERTY:{
            
        }
    }
    
    // For compound assignments we need another deoptimization point after the
    // variable/property load.
    if (node->is_compound()) {
        //            AccumulatorValueContext context(this);
        switch (assign_type) {
            case VARIABLE:
                EmitVariableLoad(node->target()->AsVariableProxy());
                //                    PrepareForBailout(expr->target(), TOS_REG);
                break;
            case NAMED_PROPERTY:
                //                    EmitNamedPropertyLoad(property);
                //                    PrepareForBailoutForId(property->LoadId(), TOS_REG);
                break;
            case KEYED_PROPERTY:
                //                    EmitKeyedPropertyLoad(property);
                //                    PrepareForBailoutForId(property->LoadId(), TOS_REG);
                break;
        }
        
        Token::Value op = node->binary_op();
        //        assert(op == v8::internal::Token::ADD);
        //        __ Push(rax);  // Left operand goes on the stack.
        VisitStartStackAccumulation(node->value());
       //            EmitBinaryOp(expr->binary_operation(), op, mode);
    }
        //TODO : look into this:
    
    VisitStartAccumulation(node->value());
    
    // store the value.
    switch (assign_type) {
        case VARIABLE: {
            VariableProxy *target = (VariableProxy *)node->target();
            assert(target);
            std::string str = stringFromV8AstRawString(target->raw_name());
            llvm::Function *f = _builder->GetInsertBlock()->getParent();
            llvm::AllocaInst *alloca = CreateEntryBlockAlloca(f, str);
            _namedValues[str] = alloca;
            
            EmitVariableAssignment(node->target()->AsVariableProxy()->var(), node->op());
           
            llvm::Value *rhs = PopContext();
            if (rhs) {
                _builder->CreateStore(rhs, alloca);
            } else {
               //TODO :
            }
            
            break;
            
        } case NAMED_PROPERTY: {
            //            EmitNamedPropertyAssignment(expr);
            break;
        }
        case KEYED_PROPERTY: {
            //            EmitKeyedPropertyAssignment(expr);
            break;
        }
    }
}
void ObjCCodeGen::EmitBinaryOp(BinaryOperation* expr, Token::Value op){
//TODO:
}

void ObjCCodeGen::EmitVariableAssignment(Variable* var, Token::Value op) {
//TODO:
}

void ObjCCodeGen::EmitVariableLoad(VariableProxy* node) {
    std::string str = stringFromV8AstRawString(node->raw_name());
    auto varValue = _namedValues[str];
    PushValueToContext(_builder->CreateLoad(varValue, str));
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
    Expression *callee = node->expression();
    Call::CallType callType = GetCallType(node, isolate());
   
    std::string name;
    
    if (callType == Call::GLOBAL_CALL) {
        assert(0 == "unimplemented");
    } else if (callType == Call::LOOKUP_SLOT_CALL) {
        VariableProxy* proxy = callee->AsVariableProxy();
         name = stringFromV8AstRawString(proxy->raw_name());
    } else if (callType == Call::OTHER_CALL){
        VariableProxy* proxy = callee->AsVariableProxy();
        name = stringFromV8AstRawString(proxy->raw_name());
        
    } else {
        VariableProxy* proxy = callee->AsVariableProxy();
        name = stringFromV8AstRawString(proxy->raw_name());
    }
   
    auto *calleeF = _module->getFunction(name);
    
    if (calleeF == 0){
        //TODO: this would call an objc or c function in the future
        printf("Unknown function referenced");
        return;
    }

    ZoneList<Expression*>* args = node->arguments();
   
    if (calleeF->arg_size() != args->length()) {
        printf("Unknown function referenced: airity mismatch");
        abort();
    }
    
    for (int i = 0; i <args->length(); i++) {
        VisitStartAccumulation(args->at(i));
    }
   
    if (_accumulatorContext->back() == 0) {
        return;
    }
    
    PushValueToContext(_builder->CreateCall(calleeF, *_accumulatorContext, "calltmp"));
    
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

}


void ObjCCodeGen::VisitBinaryOperation(BinaryOperation* expr) {
  switch (expr->op()) {
    case Token::COMMA:
      return VisitComma(expr);
    case Token::OR:
    case Token::AND:
      return VisitLogicalExpression(expr);
    default:
      return VisitArithmeticExpression(expr);
  }
}

void ObjCCodeGen::VisitComma(BinaryOperation* expr) {
    
}

void ObjCCodeGen::VisitLogicalExpression(BinaryOperation* expr) {
    
}

void ObjCCodeGen::VisitArithmeticExpression(BinaryOperation* expr) {
    Expression *left = expr->left();
    Expression *right = expr->right();
   
    VisitStartStackAccumulation(left);
    auto lhs = PopContext();
    
    VisitStartAccumulation(right);
    auto rhs = PopContext();
   
    llvm::Value *result = NULL;
    auto op = expr->op();
    switch (op) {
        case v8::internal::Token::ADD : {
            result = _builder->CreateFAdd(lhs, rhs, "addtmp");
            break;
        }
        case v8::internal::Token::SUB : {
            result = _builder->CreateFSub(lhs, rhs, "subtmp");
            break;
        }
//        case '*': return Builder.CreateFMul(L, R, "multmp");
//        case '<':
//            L = Builder.CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
//            return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()),
//                                        "booltmp");
        default: break;
    }
    
    PushValueToContext(result);
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
    if (!_accumulatorContext) {
        _accumulatorContext = new std::vector<llvm::Value *>;
    }
    _context = _accumulatorContext;
    Visit(expr);
}

void ObjCCodeGen::EndAccumulation() {
    delete _accumulatorContext;
    _accumulatorContext = NULL;
}

void ObjCCodeGen::VisitStartStackAccumulation(Expression *expr) {
    if (!_stackAccumulatorContext) {
        _stackAccumulatorContext = new std::vector<llvm::Value *>;
    }
    _context = _stackAccumulatorContext;
    Visit(expr);
}

void ObjCCodeGen::EndStackAccumulation(){
    delete _stackAccumulatorContext;
    _stackAccumulatorContext = NULL;
}

void ObjCCodeGen::PushValueToContext(llvm::Value *value) {
    _context->push_back(value);
}

llvm::Value *ObjCCodeGen::PopContext() {
    auto value = _context->back();
    _context->pop_back();
    return value;
}
