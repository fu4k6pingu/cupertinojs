//
//  cgobjcjsclang.cpp
//  objcjs
//
//  Created by Jerry Marino on 8/2/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#include "objcjsutils.h"
#include "cgobjcjsclang.h"
#include <clang-c/Index.h>

#define CGObjCJSDEBUG 0
#define ILOG(A, ...) if (CGObjCJSDEBUG){ printf(A,##__VA_ARGS__), printf("\n");}

using namespace objcjs;

ObjCClass::ObjCClass(std::string name) {
    _name = name;
}

void printDiagnostics(CXTranslationUnit translationUnit);
void printTokenInfo(CXTranslationUnit translationUnit,CXToken currentToken);
void printCursorTokens(CXTranslationUnit translationUnit,CXCursor currentCursor);

CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data);
CXChildVisitResult functionDeclVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data);

static std::string ENV_PROJECT_ROOT_DIR("OBJCJS_ENV_PROJECT_ROOT_DIR");

ClangFile::ClangFile(std::string name) {
    _name = name;
    _currentClass = NULL;
    
    CXIndex index = clang_createIndex(0, 0);

    const char *args[] = {
        "-I/usr/include",
        "-I.",
        "-c",
        "-x",
        "objective-c"
    };
   
    std::string rootDir = get_env_var(ENV_PROJECT_ROOT_DIR);
    std::string file = string_format("%s%s", rootDir.c_str(), name.c_str());
   
    int numArgs = sizeof(args) / sizeof(*args);
    CXTranslationUnit translationUnit = clang_parseTranslationUnit(index, file.c_str(), args, numArgs, NULL, 0, CXTranslationUnit_None);
    CXCursor rootCursor = clang_getTranslationUnitCursor(translationUnit);

    clang_visitChildren(rootCursor, *cursorVisitor, this);
    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);
    
    assert(translationUnit);
}

ClangFile::~ClangFile() {
    delete _currentClass;
    delete _currentMethod;
}

void printDiagnostics(CXTranslationUnit translationUnit){
    int nbDiag = clang_getNumDiagnostics(translationUnit);
    ILOG("There is %i diagnostics\n",nbDiag);
    
    for (unsigned int currentDiag = 0; currentDiag < nbDiag; ++currentDiag) {
        CXDiagnostic diagnotic = clang_getDiagnostic(translationUnit, currentDiag);
        CXString errorString = clang_formatDiagnostic(diagnotic,clang_defaultDiagnosticDisplayOptions());
        ILOG("%s\n", clang_getCString(errorString));
        clang_disposeString(errorString);
    }
}

void printTokenInfo(CXTranslationUnit translationUnit,CXToken currentToken) {
    CXString tokenString = clang_getTokenSpelling(translationUnit, currentToken);
    CXTokenKind kind = clang_getTokenKind(currentToken);
    
    switch (kind) {
        case CXToken_Comment:
            ILOG("Token : %s \t| COMMENT\n", clang_getCString(tokenString));
            break;
        case CXToken_Identifier:
            ILOG("Token : %s \t| IDENTIFIER\n", clang_getCString(tokenString));
            break;
        case CXToken_Keyword:
            ILOG("Token : %s \t| KEYWORD\n", clang_getCString(tokenString));
            break;
        case CXToken_Literal:
            ILOG("Token : %s \t| LITERAL\n", clang_getCString(tokenString));
            break;
        case CXToken_Punctuation:
            ILOG("Token : %s \t| PUNCTUATION\n", clang_getCString(tokenString));
            break;
        default:
            break;
    }
}

void printCursorTokens(CXTranslationUnit translationUnit, CXCursor currentCursor) {
    CXToken *tokens;
    unsigned int nbTokens;
    CXSourceRange srcRange;
    
    srcRange = clang_getCursorExtent(currentCursor);
    
    clang_tokenize(translationUnit, srcRange, &tokens, &nbTokens);
    
    for (int i = 0; i < nbTokens; ++i) {
        CXToken currentToken = tokens[i];
        printTokenInfo(translationUnit,currentToken);
    }
    
    clang_disposeTokens(translationUnit,tokens,nbTokens);
}

CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data){
    CXCursorKind kind = clang_getCursorKind(cursor);
    CXString name = clang_getCursorSpelling(cursor);
    ILOG("cursor '%s' kind %d\n",clang_getCString(name), kind);
    
    ClangFile *file = (ClangFile *)client_data;
    
    if (kind == CXCursor_ObjCInterfaceDecl) {
        std::string className = clang_getCString(name);
        ILOG("\tclass '%s'\n",clang_getCString(name));
        ObjCClass *currentClass = new ObjCClass(className);
        file->_classes.insert(currentClass);
        file->_currentClass = currentClass;
        return CXChildVisit_Continue;
    }
    
    if (kind == CXCursor_ObjCInstanceMethodDecl && file->_currentClass){
        ObjCMethod *method = new ObjCMethod;
        method->name = clang_getCString(name);

        auto retType = clang_getCursorResultType(cursor);
        method->type = (ObjCType)retType.kind;
        
        file->_currentMethod = method;
        ILOG("method '%s'\n",method->name.c_str());
       
        // visit method childs
        clang_visitChildren(cursor, *functionDeclVisitor,file);
        ILOG("nb Params : %lu'\n", method->params.size());
        file->_currentClass->_methods.push_back(method);
        return CXChildVisit_Continue;
    }
    
    return CXChildVisit_Recurse;
}

CXChildVisitResult functionDeclVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data) {
    CXCursorKind kind = clang_getCursorKind(cursor);
    CXType type = clang_getCursorType(cursor);

    ClangFile *file = (ClangFile *)client_data;
    
    if (kind == CXCursor_ParmDecl){
        CXString name = clang_getCursorSpelling(cursor);
        auto param = (ObjCMethodParam){clang_getCString(name), (ObjCType)type.kind};
        file->_currentMethod->params.push_back(param);
        ILOG("\tparameter: '%s' of type '%i'\n", param.name.c_str(), param.type);
    }
    
    return CXChildVisit_Continue;
}

