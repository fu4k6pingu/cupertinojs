//
//  cgclang.h
//  cujs
//
//  Created by Jerry Marino on 8/2/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#ifndef __cujs__cgjsclang__
#define __cujs__cgjsclang__

#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <set>
#include <map>

namespace cujs {
    typedef enum ObjCType {
        /**
         * \brief Reprents an invalid type (e.g., where no type is available).
         */
        ObjCType_Invalid = 0,
        
        /**
         * \brief A type whose specific kind is not exposed via this
         * interface.
         */
        ObjCType_Unexposed = 1,
        
        /* Builtin types */
        ObjCType_Void = 2,
        ObjCType_Bool = 3,
        ObjCType_Char_U = 4,
        ObjCType_UChar = 5,
        ObjCType_Char16 = 6,
        ObjCType_Char32 = 7,
        ObjCType_UShort = 8,
        ObjCType_UInt = 9,
        ObjCType_ULong = 10,
        ObjCType_ULongLong = 11,
        ObjCType_UInt128 = 12,
        ObjCType_Char_S = 13,
        ObjCType_SChar = 14,
        ObjCType_WChar = 15,
        ObjCType_Short = 16,
        ObjCType_Int = 17,
        ObjCType_Long = 18,
        ObjCType_LongLong = 19,
        ObjCType_Int128 = 20,
        ObjCType_Float = 21,
        ObjCType_Double = 22,
        ObjCType_LongDouble = 23,
        ObjCType_NullPtr = 24,
        ObjCType_Overload = 25,
        ObjCType_Dependent = 26,
        ObjCType_ObjCId = 27,
        ObjCType_ObjCClass = 28,
        ObjCType_ObjCSel = 29,
        ObjCType_FirstBuiltin = ObjCType_Void,
        ObjCType_LastBuiltin  = ObjCType_ObjCSel,
        
        ObjCType_Complex = 100,
        ObjCType_Pointer = 101,
        ObjCType_BlockPointer = 102,
        ObjCType_LValueReference = 103,
        ObjCType_RValueReference = 104,
        ObjCType_Record = 105,
        ObjCType_Enum = 106,
        ObjCType_Typedef = 107,
        ObjCType_ObjCInterface = 108,
        ObjCType_ObjCObjectPointer = 109,
        ObjCType_FunctionNoProto = 110,
        ObjCType_FunctionProto = 111,
        ObjCType_ConstantArray = 112,
        ObjCType_Vector = 113,
        ObjCType_IncompleteArray = 114,
        ObjCType_VariableArray = 115,
        ObjCType_DependentSizedArray = 116,
        ObjCType_MemberPointer = 117
    } ObjCType;
  
    typedef struct ObjCMethodParam {
        std::string name;
        ObjCType type;
    } ObjCMethodParam;
    
    typedef struct ObjCMethod {
        std::string name;
        std::vector <ObjCMethodParam>params;
        ObjCType type;
    } ObjCMethod;
   
    class ObjCClass {
    public:
        std::string _name;
        std::vector<ObjCMethod *>_methods;
        ObjCClass(std::string _name);
    };
    
    typedef struct ObjCField {
    public:
        std::string name;
        std::string typeName;
        unsigned long offset;
        ObjCType type;
    } ObjCField;
    
    typedef struct ObjCStruct {
    public:
        std::string name;
        std::vector <ObjCField>fields;
        ObjCType type;
        unsigned long size;
    } ObjCStruct;
   
    typedef struct ObjCTypeDef {
    public:
        std::string name;
        std::string typeDefName;
        std::string encodingString;
        ObjCType type;
    } ObjCTypeDef;
    
    
    // ClangFile is is a representation of useful details of
    // a source file (currently supports headers only)
    class ClangFile {
        std::string _name;
    public:
        std::set <ObjCClass *> _classes;
        std::set <ObjCStruct *> _structs;
        std::set <ObjCTypeDef *> _typeDefs;
        ObjCClass *_currentClass;
        ObjCMethod *_currentMethod;
        ObjCStruct *_currentStruct;
        long long _curentOffset;
        
        //Import a c language file into the module
        ClangFile(std::string name);
        ~ClangFile();
    };
}


#endif /* defined(__cujs__cgjsclang__) */
