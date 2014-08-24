//
//  cgjsruntime.cpp
//  cujs
//
//  Created by Jerry Marino on 7/19/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include <malloc/malloc.h>
#include <string>

#include "cgjsruntime.h"
#include "cgclang.h"
#include "cujsutils.h"

using namespace cujs;
using namespace llvm;

#define DEBUG 0
#define ILOG(A, ...) if (DEBUG){ printf(A,##__VA_ARGS__), printf("\n");}

#define debug_print(fmt, ...) \
do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
__LINE__, __func__, __VA_ARGS__); } while (0)

std::string cujs::ObjCSelectorToJS(std::string objCSelector){
    std::string jsSelector;
    size_t length = objCSelector.length();
    int remove = 0;
    int jsSelectorPos = 0;
    for (int i = 0; i < length; i++) {
        char current = objCSelector.at(i);
        if (current == ':'){
            i++;
            remove++;
            if (i == length) {
                break;
            }
            current = objCSelector.at(i);
            jsSelector += toupper(current);
        } else {
            jsSelector += current;
        }
        jsSelectorPos++;
    }

    return jsSelector;
}

const auto _ObjcPointerTy = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);

llvm::Type *cujs::ObjcPointerTy(){
    return _ObjcPointerTy;
}

llvm::Value *cujs::ObjcNullPointer() {
    auto ty = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);
    return llvm::ConstantPointerNull::get(ty);
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
llvm::AllocaInst *cujs::CreateEntryBlockAlloca(llvm::Function *Function,
                                          const std::string &VarName) {
    llvm::IRBuilder<> Builder(&Function->getEntryBlock(),
                     Function->getEntryBlock().begin());
    return Builder.CreateAlloca(ObjcPointerTy(), 0, VarName.c_str());
}

llvm::Function *ObjcCodeGenFunction(size_t numParams,
                                    std::string name,
                                    llvm::Module *mod) {
    std::vector<llvm::Type*> Params(numParams, ObjcPointerTy());
    llvm::FunctionType *FT = llvm::FunctionType::get(ObjcPointerTy(), Params, false);
    return llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, mod);
}

llvm::Function *ObjcCodeGenFunction(size_t numParams,
                                    std::string name,
                                    llvm::Module *mod,
                                    bool isVarArg) {
    std::vector<llvm::Type*> Params(numParams, ObjcPointerTy());
    llvm::FunctionType *FT = llvm::FunctionType::get(ObjcPointerTy(), Params, true);
    return llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, mod);
}


llvm::Function *cujs::CGJSFunction(size_t numParams,
                                 std::string name,
                                 llvm::Module *mod) {
    std::vector<llvm::Type*> Params;

    Params.push_back(ObjcPointerTy());
    Params.push_back(ObjcPointerTy());
    
    for (int i = 0; i < numParams; i++) {
        Params.push_back(ObjcPointerTy());
    }
    
    llvm::FunctionType *FT = llvm::FunctionType::get(ObjcPointerTy(), Params, true);
    return llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, mod);
}

llvm::Function *cujs::ObjcCodeGenModuleInit(llvm::IRBuilder<>*builder,
                                      llvm::Module *module,
                                      std::string name) {
    std::vector<llvm::Type*> argumentTypes;
    argumentTypes.push_back(ObjcPointerTy());
    llvm::FunctionType *functionType = llvm::FunctionType::get(
                                                        ObjcPointerTy(),
                                                        argumentTypes,
                                                        false);
    
    llvm::Function *function = llvm::Function::Create(
                                                  functionType,
                                                  llvm::Function::ExternalLinkage,
                                                  llvm::Twine(name),
                                                  module);
    
    function->setCallingConv(llvm::CallingConv::C);
    
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(module->getContext(), "entry", function);
    builder->SetInsertPoint(BB);
   
    builder->saveAndClearIP();
    return function;
}

// SetModuleCtor - this is the constructor for the module
void cujs::SetModuleCtor(llvm::Module *module, llvm::Function *cTor){
    llvm::Function *func_init = module->getFunction("cujs_module_init");
    assert(!func_init && "module already has a ctor");
    
    std::vector<llvm::Type*>FuncTy_args;
    llvm::FunctionType *FuncTy = llvm::FunctionType::get(
                                                         llvm::Type::getVoidTy(module->getContext()),
                                                         FuncTy_args,
                                                         false);
    func_init = llvm::Function::Create(
                                       FuncTy,
                                       llvm::GlobalValue::ExternalLinkage,
                                       "cujs_module_init", module);
    func_init->setCallingConv(llvm::CallingConv::C);
    
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(module->getContext(), "entry", func_init);
    
    std::vector<llvm::Value *>Args;
    Args.push_back(ObjcNullPointer());
    auto callModuleFunction = llvm::CallInst::Create(cTor,Args);

    BB->getInstList().push_front(callModuleFunction);
    llvm::ReturnInst::Create(module->getContext(), BB);
    
    std::vector<llvm::Type*>StructTy_fields;
    StructTy_fields.push_back(llvm::IntegerType::get(module->getContext(), 32));
    StructTy_fields.push_back(func_init->getType());
    
    llvm::StructType *StructTy = llvm::StructType::get(module->getContext(), StructTy_fields, false);
    std::vector<llvm::Constant*> const_array_elems;

    std::vector<llvm::Constant*> const_struct_fields;
    llvm::ConstantInt *const_int32_99 = llvm::ConstantInt::get(module->getContext(),llvm:: APInt(32, llvm::StringRef("65535"), 10));
    const_struct_fields.push_back(const_int32_99);
    const_struct_fields.push_back(func_init);
    llvm::Constant *const_struct = llvm::ConstantStruct::get(StructTy, const_struct_fields);
    
    const_array_elems.push_back(const_struct);
    
    llvm::ArrayType *ArrayTy = llvm::ArrayType::get(StructTy, 1);
    llvm::Constant *const_array = llvm::ConstantArray::get(ArrayTy, const_array_elems);
    llvm::GlobalVariable *gvar_array_llvm_global_ctors = new llvm::GlobalVariable(*module,
                                                                                  ArrayTy,
                                                                                  false,
                                                                                  llvm::GlobalValue::AppendingLinkage,
                                                                                  0,
                                                                                  "llvm.global_ctors");
    gvar_array_llvm_global_ctors->setInitializer(const_array);
}

#define DefExternFucntion(name){\
{\
    std::vector<llvm::Type*> ArgumentTypes; \
    ArgumentTypes.push_back(ObjcPointerTy()); \
    llvm::FunctionType *ft = llvm::FunctionType::get(ObjcPointerTy(), ArgumentTypes, true); \
    auto function = llvm::Function::Create( \
        ft, llvm::Function::ExternalLinkage, \
        llvm::Twine(name), \
        _module );\
    function->setCallingConv(llvm::CallingConv::C); \
} \
}

static llvm::Function *ObjcMsgSendFPret(llvm::Module *module) {
        std::vector<llvm::Type*> ArgumentTypes;
    ArgumentTypes.push_back(ObjcPointerTy());
    auto doubleTy  = llvm::Type::getDoubleTy(llvm::getGlobalContext());
    llvm::FunctionType *ft = llvm::FunctionType::get(doubleTy, ArgumentTypes, true); \
    auto function = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage,
                                           llvm::Twine("objc_msgSend_fpret"), \
        module );\
    function->setCallingConv(llvm::CallingConv::C);
    return function;
}

static llvm::Function *ObjcNSLogPrototye(llvm::Module *module)
{
    std::vector<llvm::Type*> ArgumentTypes;
    ArgumentTypes.push_back(ObjcPointerTy());
    
    llvm::FunctionType *functionType =
    llvm::FunctionType::get(
                            llvm::Type::getInt32Ty(module->getContext()), ArgumentTypes, true);
    
    llvm::Function *function = llvm::Function::Create(
                                                  functionType, llvm::Function::ExternalLinkage,
                                                  llvm::Twine("NSLog"),
                                                  module
                                                  );
    function->setCallingConv(llvm::CallingConv::C);
    return function;
}

static llvm::Function *ObjcMallocPrototype(llvm::Module *module) {
    std::vector<llvm::Type*>ArgumentTypes;
    ArgumentTypes.push_back(llvm::IntegerType::get(module->getContext(), 64));
    llvm::FunctionType *functionType = llvm::FunctionType::get(
                                                               ObjcPointerTy(),
                                                               ArgumentTypes,
                                                               false);
    llvm::Function *function = module->getFunction("malloc");
    if (!function) {
        function = llvm::Function::Create(
                                          functionType,
                                          llvm::GlobalValue::ExternalLinkage,
                                          "malloc", module); // (external, no body)
        function->setCallingConv(llvm::CallingConv::C);
    }
    
    return function;
}


llvm::Value *cujs::NewLocalStringVar(const char *data,
                            size_t len,
                            llvm::Module *module) {
    llvm::Constant *constTy = llvm::ConstantDataArray::getString(llvm::getGlobalContext(), data);
    auto type = llvm::ArrayType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), len + 1);
    llvm::GlobalVariable *var = new llvm::GlobalVariable(*module,
                                                         type,
                                                         true,
                                                         llvm::GlobalValue::PrivateLinkage,
                                                         0,
                                                         ".str");
    var->setInitializer(constTy);
    
    std::vector<llvm::Constant*> constStringIndicies;
    llvm::ConstantInt *constZero =llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, llvm::StringRef("0"), 10));
    constStringIndicies.push_back(constZero);
    constStringIndicies.push_back(constZero);
    llvm::Constant *constStringPtr = llvm::ConstantExpr::getGetElementPtr(var, constStringIndicies);
    auto castedConst = llvm::ConstantExpr::getPointerCast(constStringPtr, ObjcPointerTy());
    return castedConst;
}

llvm::Value *cujs::NewLocalStringVar(std::string value,
                            llvm::Module *module) {
    return cujs::NewLocalStringVar(value.c_str(), value.size(), module);
}

#pragma mark - Runtime calls

void defMemcpy(Module *_module){
    llvm::PointerType *EightBytePointerTy = llvm::PointerType::get(llvm::IntegerType::get(_module->getContext(), 8), 0);
    std::vector<llvm::Type*>ArgumentTypes;
    ArgumentTypes.push_back(EightBytePointerTy);
    ArgumentTypes.push_back(EightBytePointerTy);
    ArgumentTypes.push_back(llvm::IntegerType::get(_module->getContext(), 64));
    ArgumentTypes.push_back(llvm::IntegerType::get(_module->getContext(), 32));
    ArgumentTypes.push_back(llvm::IntegerType::get(_module->getContext(), 1));
    llvm::FunctionType *FuncTy_6 = llvm::FunctionType::get(
                                                           /*Result=*/llvm::Type::getVoidTy(_module->getContext()),
                                                           /*Params=*/ArgumentTypes,
                                                           /*isVarArg=*/false);
    llvm::Function *func_llvm_memcpy_p0i8_p0i8_i64 = llvm::Function::Create(
                                                            /*Type=*/FuncTy_6,
                                                            /*Linkage=*/llvm::GlobalValue::ExternalLinkage,
                                                            /*Name=*/"llvm.memcpy.p0i8.p0i8.i64", _module); // (external, no body)
    func_llvm_memcpy_p0i8_p0i8_i64->setCallingConv(llvm::CallingConv::C);
}
CGJSRuntime::CGJSRuntime(llvm::IRBuilder<> *builder,
                                 llvm::Module *module){
    _builder = builder;
    _module = module;
   
    _builtins.insert("NSLog");
    DefExternFucntion("objc_Struct");
    _builtins.insert("objc_Struct");
    
    DefExternFucntion("sel_getUid");
    DefExternFucntion("sel_getName");
    DefExternFucntion("objc_getClass");

    //JSRuntime
    DefExternFucntion("cujs_invoke");
    DefExternFucntion("cujs_defineJSFunction");
    DefExternFucntion("cujs_defineStruct");
    DefExternFucntion("objc_msgSend_stret");
  
    ObjcCodeGenFunction(0, "cujs_newJSObjectClass", _module);
    ObjcCodeGenFunction(3, "cujs_assignProperty", _module);

    //Objc runtim
    ObjcCodeGenFunction(0, "objc_msgSend", _module, true);
    
    ObjcMsgSendFPret(_module);
    ObjcMallocPrototype(_module);
    ObjcNSLogPrototye(_module);
}

llvm::Value *CGJSRuntime::newString(std::string string){
    const char *name = string.c_str();
    llvm::Value *strGlobal = NewLocalStringVar(name, string.length(), _module);
    return messageSend(classNamed("NSString"), "stringWithUTF8String:", strGlobal);
}

llvm::Value *CGJSRuntime::classNamed(const char *name){
    return _builder->CreateCall(_module->getFunction("objc_getClass"),  NewLocalStringVar(std::string(name), _module), "calltmp-getclass");
}

llvm::Value *CGJSRuntime::newNumber(double doubleValue){
    auto constValue = llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(doubleValue));
    return messageSend(classNamed("NSNumber"), "numberWithDouble:", constValue);
}

llvm::Value *CGJSRuntime::newObject() {
    auto newClass = _builder->CreateCall(_module->getFunction("cujs_newJSObjectClass"));
    return messageSend(newClass, "cujs_new");
}

llvm::Value *CGJSRuntime::newObject(std::vector<llvm::Value *>values) {
    UNIMPLEMENTED();
    auto newClass = _builder->CreateCall(_module->getFunction("cujs_newJSObjectClass"));
    return messageSend(newClass, "cujs_withObjectsAndKeys:", values);
}

llvm::Value *CGJSRuntime::newArray(std::vector<llvm::Value *>values) {
    return messageSend(classNamed("CUJSMutableArray"), "arrayWithObjects:", values);
}

llvm::Value *CGJSRuntime::newNumberWithLLVMValue(llvm::Value *doubleValue){
    return messageSend(classNamed("NSNumber"), "numberWithDouble:", doubleValue);
}

llvm::Value *CGJSRuntime::doubleValue(llvm::Value *llvmValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), NewLocalStringVar(std::string("doubleValue"), _module), "calltmp");

    std::vector<llvm::Value*> Args;
    Args.push_back(llvmValue);
    Args.push_back(sel);

    return _builder->CreateCall(_module->getFunction("objc_msgSend_fpret"), Args, "calltmp-objc_msgSend_fpret");
}

llvm::Value *CGJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *selectorName,
                                          llvm::Value *Arg) {
    llvm::Value *selector = _builder->CreateCall(_module->getFunction("sel_getUid"), NewLocalStringVar(std::string(selectorName), _module), "calltmp");
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selector);
   
    if (Arg){
        Args.push_back(Arg);
    }
   
    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "calltmp-objc_msgSend");
}

llvm::Value *CGJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *selectorName,
                                          std::vector<llvm::Value *>ArgsV) {
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selectorWithName(selectorName));
    for (int i = 0; i < ArgsV.size(); i++) {
        Args.push_back(ArgsV.at(i));
    }
  
    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "calltmp-objc_msgSend");

}

llvm::Value *CGJSRuntime::messageSendFP(llvm::Value *receiver,
                                          const char *selectorName,
                                          std::vector<llvm::Value *>ArgsV) {
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selectorWithName(selectorName));
    for (int i = 0; i < ArgsV.size(); i++) {
        Args.push_back(ArgsV.at(i));
    }
  
    return _builder->CreateCall(_module->getFunction("objc_msgSend_fpret"), Args, "calltmp-objc_msgSend");
}

llvm::Value *CGJSRuntime::messageSend(llvm::Value *receiver,
                                          const char *name) {
    std::vector<llvm::Value*> Args;
    Args.push_back(receiver);
    Args.push_back(selectorWithName(name));
    llvm::Function *function = _module->getFunction("objc_msgSend");
    return _builder->CreateCall(function, Args, "calltmp-objc_msgSend");
}

llvm::Value *CGJSRuntime::selectorWithName(const char *name){
    return _builder->CreateCall(_module->getFunction("sel_getUid"), NewLocalStringVar(std::string(name), _module), "calltmp");
}

llvm::Value *CGJSRuntime::messageSendProperty(llvm::Value *receiver,
                                 const char *name,
                                 std::vector<llvm::Value *>ArgsV) {
    return messageSend(receiver, name, ArgsV);
}

//Sends a message to a JSFunction instance
llvm::Value *CGJSRuntime::invokeJSValue(llvm::Value *instance,
                                                    std::vector<llvm::Value *>ArgsV) {
    std::vector<llvm::Value*> Args;
    Args.push_back(instance);

    for (int i = 0; i < ArgsV.size(); i++) {
        Args.push_back(ArgsV.at(i));
    }

    Args.push_back(ObjcNullPointer());
    return _builder->CreateCall(_module->getFunction("cujs_invoke"), Args, "cujs_invoke");
}

//Convert a llvm value to an bool
//which is represented as an int in JS land
llvm::Value *CGJSRuntime::boolValue(llvm::Value *llvmValue){
    llvm::Value *sel = _builder->CreateCall(_module->getFunction("sel_getUid"), NewLocalStringVar(std::string("cujs_boolValue"), _module), "calltmp");

    std::vector<llvm::Value*> Args;
    Args.push_back(llvmValue);
    Args.push_back(sel);

    return _builder->CreateCall(_module->getFunction("objc_msgSend"), Args, "objc_msgSend");
}

llvm::Value *CGJSRuntime::defineJSFuction(const char *name,
                                              unsigned nArgs
                                                 ) {
    auto function = CGJSFunction(nArgs, name, _module);
    
    auto nameAlloca = NewLocalStringVar(name, strlen(name), _module);
    
    std::vector<llvm::Value*> Args;
    Args.push_back(nameAlloca);
    Args.push_back(function);

    return llvm::CallInst::Create(_module->getFunction("cujs_defineJSFunction"), Args, "calltmp");
}

llvm::Value *CGJSRuntime::declareProperty(llvm::Value *instance,
                                              std::string name) {
    
    auto nameAlloca = NewLocalStringVar(name.c_str(), name.length(), _module);
    return messageSend(instance, "cujs_defineProperty:", nameAlloca);
}

llvm::Value *CGJSRuntime::assignProperty(llvm::Value *instance,
                                             std::string name,
                                             llvm::Value *value) {
    auto nameAlloca = NewLocalStringVar(name.c_str(), name.length(), _module);
    std::vector<llvm::Value*> Args;
    Args.push_back(instance);
    Args.push_back(nameAlloca);
    Args.push_back(value);
    
    return _builder->CreateCall(_module->getFunction("cujs_assignProperty"), Args, "calltmp");
}

llvm::Value *CGJSRuntime::declareGlobal(std::string name) {
    llvm::GlobalVariable *var = new llvm::GlobalVariable(*_module,
                                                         ObjcPointerTy(),
                                                         false,
                                                         llvm::GlobalValue::ExternalLinkage,
                                                         0,
                                                         name);
    auto ty = llvm::PointerType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), 4);
    var->setInitializer(llvm::ConstantPointerNull::get(ty));
    return var;
}

llvm::Constant *ConstantCharValue(Module *module, std::string name){
   llvm::Constant *constTy = llvm::ConstantDataArray::getString(llvm::getGlobalContext(), name.c_str());
    auto type = llvm::ArrayType::get(llvm::IntegerType::get(llvm::getGlobalContext(), 8), name.length() + 1);
    llvm::GlobalVariable *stringVar = new llvm::GlobalVariable(*module,
                                                         type,
                                                         true,
                                                         llvm::GlobalValue::PrivateLinkage,
                                                         0,
                                                         ".str");
    stringVar->setInitializer(constTy);
    std::vector<llvm::Constant*> constStringIndicies;
    llvm::ConstantInt *constZero =llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, llvm::StringRef("0"), 10));
    constStringIndicies.push_back(constZero);
    constStringIndicies.push_back(constZero);
    llvm::Constant *constStringPtr = llvm::ConstantExpr::getGetElementPtr(stringVar, constStringIndicies);
    return constStringPtr;
}

llvm::Constant *ConstantStruct_FieldInfo(CGJSRuntime *runtime, ObjCField *field){
    Module *module = runtime->_module;
    std::string name = field->name;
    std::string typeName = field->typeName;
    auto offset = (int)field->offset;
   
    ObjCType encoding;
    auto typedefType = runtime->_typedefs[typeName];
    if (typedefType && typedefType->type != ObjCType_Unexposed) {
        ILOG("typdefed type %s to %d \n", name.c_str(), typedefType->type);
        encoding = typedefType->type;
    } else {
        encoding = field->type;
    }
    
    StructType *structTyFieldInfo = module->getTypeByName("FieldInfo");
    std::vector<Constant*> constStructFields;

    Constant *constNameValue = ConstantCharValue(module, name);
    constStructFields.push_back(constNameValue);

    Constant *constTypeNameValue = ConstantCharValue(module, typeName);
    constStructFields.push_back(constTypeNameValue);
    
    ConstantInt *constOffsetValue = ConstantInt::get(module->getContext(), APInt(32, StringRef(std::to_string(offset)), 10));
    constStructFields.push_back(constOffsetValue);
   
    ConstantInt *constEncodingValue = ConstantInt::get(module->getContext(), APInt(32, StringRef(std::to_string(encoding)), 10));
    constStructFields.push_back(constEncodingValue);
    return ConstantStruct::get(structTyFieldInfo, constStructFields);
}

//FIXME : don't make everything a float or whatever floats your boat
llvm::Type *typeByObjCType(ObjCType objcType, llvm::Module *module){
//    std::map <ObjCType, llvm::Type *>types;
    return llvm::Type::getFloatTy(module->getContext());
}

llvm::Type *structTypeWithStructModule(ObjCStruct *objCStructTy,  llvm::Module *module, CGJSRuntime *runtime) {
    if (!objCStructTy) {
        return NULL;
    }
    
    std::string name = objCStructTy->name;
    if (!name.length()){
        return NULL;
    }

    std::string defName = string_format("%s", name.c_str());
    llvm::StructType *StructTy = module->getTypeByName(defName);
    if (!StructTy) {
        StructTy = llvm::StructType::create(module->getContext(), defName);
      
        std::vector<llvm::Type*>StructTy_fields;
        for (unsigned i = 0; i < objCStructTy->fields.size(); i++) {
            ObjCField field = objCStructTy->fields[i];
            ObjCTypeDef *typedefdField = runtime->_typedefs[field.typeName];

            llvm::Type *fieldType;
            if (typedefdField && typedefdField->name.length()) {
                ObjCStruct *fieldStruct = runtime->_objCStructByName[typedefdField->name];
                if (fieldStruct) {
                    fieldType = structTypeWithStructModule(fieldStruct, module, runtime);
                } else {
                    fieldStruct = runtime->_objCStructByName[field.typeName];
                    fieldType = structTypeWithStructModule(fieldStruct, module, runtime);
                }
            } else {
                fieldType = typeByObjCType(field.type, module);
            }
            
            if (!fieldType){
                ILOG("warning - missed field for deffname %s field %s \n", defName.c_str(), field.name.c_str());
                fieldType = typeByObjCType(field.type, module);
            }
    
            assert(StructTy->isValidElementType(fieldType));
            StructTy_fields.push_back(fieldType);
        }
       
        if (!StructTy_fields.size()) {
            return NULL;
        }
        if (StructTy->isOpaque()) {
            StructTy->setBody(StructTy_fields, /*isPacked=*/false);
        } else {
            abort();
        }
    }
    
    return StructTy;
}

void CGJSRuntime::enterStruct(ObjCStruct *newStruct) {
    std::string name = newStruct->name;
   
    if (!name.length() || _structs.count(name)) {
        return;
    }
    
    if (_classes.count(name) && !_structs.count(name)) {
        ILOG("warning - class exists for struct %s", name.c_str());
    }

    StructType *structTyFieldInfo = _module->getTypeByName("FieldInfo");
    if (!structTyFieldInfo) {
        structTyFieldInfo = StructType::create(_module->getContext(), "FieldInfo");
        std::vector<Type*>structTyFieldInfo_fields;
        PointerType *PointerTy = PointerType::get(IntegerType::get(_module->getContext(), 8), 0);
        
        structTyFieldInfo_fields.push_back(PointerTy);
        structTyFieldInfo_fields.push_back(PointerTy);
        structTyFieldInfo_fields.push_back(IntegerType::get(_module->getContext(), 32));
        structTyFieldInfo_fields.push_back(IntegerType::get(_module->getContext(), 32));
        if (structTyFieldInfo->isOpaque()) {
            structTyFieldInfo->setBody(structTyFieldInfo_fields, /*isPacked=*/false);
        }
    }
    
    _objCStructByName[name] = newStruct;
    std::vector<llvm::Value*> Args;
    Args.push_back(NewLocalStringVar(name, _module));

    auto fields = newStruct->fields;
    llvm::ConstantInt *constNFields =llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, llvm::StringRef(std::to_string(fields.size())), 10));
    std::vector<Constant*> constArrayElems;
    Args.push_back(constNFields);
    
    for (unsigned i = 0; i < fields.size(); i++){
        constArrayElems.push_back(ConstantStruct_FieldInfo(this, &fields[i]));
    }
    
    ArrayType *arrayTy = ArrayType::get(structTyFieldInfo, fields.size());
    Constant *constArray = ConstantArray::get(arrayTy, constArrayElems);


    GlobalVariable *gvarArray = new GlobalVariable(*_module,
                                                   arrayTy,
                                                   true,
                                                   GlobalValue::PrivateLinkage,
                                                   0,
                                                   ".str");
    gvarArray->setAlignment(4);
    gvarArray->setInitializer(constArray);
    Args.push_back(gvarArray);
    
    auto global = _module->getGlobalVariable(name);
    if (global) {
        ILOG("warning - already had global declared for struct %s \n", name.c_str());
    }
    
    llvm::Value *globalValue = declareGlobal(name);
    _builder->CreateStore(classNamed(name.c_str()), globalValue);
    _classes.insert(name);
    _structs.insert(name);
    _builder->CreateCall(_module->getFunction("cujs_defineStruct"), Args, "calltmp");
    
    structTypeWithStructModule(newStruct, _module, this);
}

void CGJSRuntime::enterClass(ObjCClass *newClass) {
    std::string name = newClass->_name;

    if (_structs.count(name)) {
        debug_print("struct exists for class %s", name.c_str());
    }

    auto global = _module->getGlobalVariable(name);
    if (global) {
        ILOG("warning - already had global declared for class %s\n", name.c_str());
    } else {
        
        NewLocalStringVar(name,  _module);
    }
    
    ILOG("Class %s #methods: %lu", name.c_str(), newClass->_methods.size());
    
    for (auto methodIt = newClass->_methods.begin(); methodIt != newClass->_methods.end(); ++methodIt){
        cujs::ObjCMethod *method = *methodIt;
        ILOG("Method (%d) %s: %lu ", method->type, method->name.c_str(), method->params.size());

        std::string objCSelector = method->name;
        auto jsSelector = ObjCSelectorToJS(objCSelector);
        
        ObjCMethod *existingMethod = ((ObjCMethod *)_objCMethodBySelector[jsSelector]);
        if(!existingMethod){
            _objCMethodBySelector[jsSelector] = (ObjCMethod *)method;
            ILOG("JS selector name %s", jsSelector.c_str());
        }
    }
    
    llvm::Value *globalValue = declareGlobal(name);
    _builder->CreateStore(classNamed(name.c_str()), globalValue);
    _classes.insert(name);
}

void CGJSRuntime::enterTypeDef(ObjCTypeDef *typeDef) {
    _typedefs[typeDef->name] = typeDef;
}

bool CGJSRuntime::isBuiltin(std::string name) {
    return _builtins.count(name) > 0;
}
