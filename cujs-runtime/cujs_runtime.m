//
//  cujs_runtime.m
//  cujs-runtime
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import "cujs_runtime.h"
#import <objc/message.h>
#import <malloc/malloc.h>

#define RuntimeException(_FMT, ...) \
    [NSException raise:@"CUJSRuntimeException" format:_FMT, ##__VA_ARGS__]

//FIXME : assert VA_ARGS if not enabled
#define CUJSRuntimeDEBUG 0
#define ILOG(A, ...){if(CUJSRuntimeDEBUG){ NSLog(A,##__VA_ARGS__), printf("\n"); }}

@implementation NSString (CUJSCString)

- (const char *)cujs_cString{
    return self.cString;
}

@end


NSString *cujs_prefixedStructClassNameWithString(NSString *original){
    return [NSString stringWithFormat:@"JSStruct%@", original];
}

NSString *cujs_prefixedStructClassNameWithCString(const char *original){
    return [NSString stringWithFormat:@"JSStruct%s", original];
}

@implementation NSObject (CUJSFunction)

static char * CUJSEnvironmentKey;

+ (id)cujs_new {
    return [[self new] autorelease];
}

- (NSMutableDictionary *)_cujs_environment {
    NSMutableDictionary *environment = objc_getAssociatedObject(self, CUJSEnvironmentKey);
    if (!environment) {
        environment = [NSMutableDictionary dictionary];
        objc_setAssociatedObject(self, CUJSEnvironmentKey, environment, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return environment;
}

static NSMutableDictionary *CUJSParents = NULL;

+ (void)_cujs_setParent:(id)parent {
    if (!CUJSParents){
        CUJSParents = [[NSMutableDictionary alloc] init];
    }

    CUJSParents[NSStringFromClass(self.class)] = parent;
}

+ (id)_cujs_parent {
    return CUJSParents[NSStringFromClass(self.class)];
}

+ (BOOL)cujs_defineProperty:(const char *)propertyName {
    SEL getterSelector = sel_getUid(propertyName);
    char *setterName = setterNameFromPropertyName(propertyName);
    SEL setterSelector = sel_getUid(setterName);
    free(setterName);
    
    if ([self respondsToSelector:getterSelector] &&
        [self respondsToSelector:setterSelector]) {
        ILOG(@"%s - already defined", __PRETTY_FUNCTION__);
        return NO;
    }
    
    return cujs_defineProperty(object_getClass(self) , propertyName);
}

- (id)_cujs_parent {
    return CUJSParents[NSStringFromClass(self.class)];
}

- (id)_cujs_body:(id)args, ... {
    return cujs_Undefined;
}

BOOL cujs_isRestrictedProperty(const char *propertyName){
    if (strcmp("version", propertyName)) {
        return NO;
    }
    return YES;
}


BOOL cujs_defineProperty(Class selfClass, const char *propertyName){
    if (cujs_isRestrictedProperty(propertyName)) {
        RuntimeException(@"unsupported - cannot define property '%s'", propertyName);
    }
    
    SEL getterSelector = sel_getUid(propertyName);
    char *setterName = setterNameFromPropertyName(propertyName);
    SEL setterSelector = sel_getUid(setterName);
    
    ILOG(@"%s: %s", __PRETTY_FUNCTION__, propertyName);
   
    IMP getter = imp_implementationWithBlock(^(id _self){
        assert(propertyName);
        id propertyNameString = [NSString stringWithCString:propertyName encoding:NSUTF8StringEncoding];
        id value;
        if ([[[_self _cujs_keyed_properties] allKeys] containsObject:propertyNameString]) {
            value = [_self _cujs_keyed_properties][propertyNameString];
        } else {
            value = objc_getAssociatedObject(_self, propertyName);
        }

        ILOG(@"ACCESS VALUE FOR KEY %@ <%p>, %s: %@", _self, _self, propertyName, value);
        return value;
    });
        
    IMP setter = imp_implementationWithBlock(^(id _self, id value){
        assert(propertyName);
        ILOG(@"SET VALUE %@ <%p>, %s - %@", _self, _self, propertyName, value);
        id propertyNameString = [NSString stringWithCString:propertyName encoding:NSUTF8StringEncoding];
        if ([[[_self _cujs_keyed_properties] allKeys] containsObject:propertyNameString]) {
            [_self _cujs_keyed_properties][propertyNameString] = value;
        } else {
            id _value = objc_getAssociatedObject(_self, propertyName);
            if (value != _value) {
                [_value release];
                objc_setAssociatedObject(_self, propertyName, value, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                ILOG(@"DID SET VALUE");
            }     
        }
    });
    
    class_addMethod(selfClass, getterSelector, getter, "@@:");
    class_addMethod(selfClass, setterSelector, setter, "v@:@");
    
    free(setterName);
    return YES;
}

- (BOOL)cujs_defineProperty:(const char *)propertyName {
    SEL getterSelector = sel_getUid(propertyName);
    char *setterName = setterNameFromPropertyName(propertyName);
    SEL setterSelector = sel_getUid(setterName);
    free(setterName);
    
    if ([self respondsToSelector:getterSelector] &&
        [self respondsToSelector:setterSelector]) {
        ILOG(@"%s - already defined", __PRETTY_FUNCTION__);
        return NO;
    }
    
    return cujs_defineProperty([self class], propertyName);
}

char *setterNameFromPropertyName(const char *propertyName){
    size_t nameLen = strlen(propertyName);
    char *setterName = malloc((sizeof(char) * nameLen) + 5);
  
    memcpy(setterName, "set", 4);
    setterName[3] = toupper(propertyName[0]);
   
    for (int i = 1; i < nameLen; i++) {
        setterName[i + 3] = propertyName[i];
    }
    
    setterName[nameLen + 3] = ':';
    setterName[nameLen + 4] = '\0';
    sel_registerName(setterName);
    return setterName;
}

- (void)_cujs_env_setValue:(id)value forKey:(NSString *)key {
    NSMutableDictionary *environment = [self _cujs_environment];
    if (!value) {
        if (![[environment allKeys] containsObject:key]) {
            [[self _cujs_parent] _cujs_env_setValue:[NSNull null] forKey:key];
        } else {
            environment[[key copy]] = [NSNull null];
        }
    } else {
        if (![[environment allKeys] containsObject:key]) {
            [[self _cujs_parent] _cujs_env_setValue:value forKey:key];
        } else {
            environment[[key copy]] = value;
        }   
    }
}

- (void)_cujs_env_setValue:(id)value declareKey:(NSString *)key {
    NSMutableDictionary *environment = [self _cujs_environment];
    if (!value) {
        environment[[key copy]] = [NSNull null];
    } else {
        environment[[key copy]] = value;
    }
}

- (id)_cujs_env_valueForKey:(NSString *)key {
    NSMutableDictionary *environment = [self _cujs_environment];
    id value = environment[key];
    return value && ![value isKindOfClass:[NSNull class]] ? value : [[self _cujs_parent] _cujs_env_valueForKey:key];
}

static const char *NSObjectPrototypeKey;

+ (id)prototype {
    CUJSPrototype *prototype = objc_getAssociatedObject(self, NSObjectPrototypeKey);
    if (!prototype) {
        prototype = [[CUJSPrototype new] retain];
        prototype.targetClass = NSClassFromString(NSStringFromClass(self.class));
        [self setPrototype:prototype];
    }
    return prototype;
}

+ (void)setPrototype:(id)prototype{
    objc_setAssociatedObject(self, NSObjectPrototypeKey, prototype, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end

@implementation NSObject (CUJSExtend)

+ (id)extend:(NSString *)name {
    return cujs_newSubclass(self.class, name);
}

@end

const char *JSFunctionTag = "JSFunctionTag";

//an imp signature is id name(_strong id, SEL, ...);
void *cujs_defineJSFunction(const char *name,
                       JSFunctionBodyIMP body){
    ILOG(@"-%s %s %p",__PRETTY_FUNCTION__, name, body);
    Class superClass = [NSObject class];
    Class jsClass = objc_allocateClassPair(superClass, name, 16);

    SEL bodySelector = @selector(_cujs_body:);
    Method superBody = class_getInstanceMethod(superClass, bodySelector);
    assert(superBody);
    const char *enc = method_getTypeEncoding(superBody);
    class_addMethod(jsClass, bodySelector, (IMP)body, enc);
    objc_registerClassPair(jsClass);
  
    //Tag object
    objc_setAssociatedObject(jsClass, JSFunctionTag, @YES, OBJC_ASSOCIATION_ASSIGN);

    //Override dealloc and retain for debugging
    Method superDealloc = class_getInstanceMethod(superClass, @selector(dealloc));
    IMP dealloc = imp_implementationWithBlock(^(id _self, SEL cmd){
        ILOG(@"DEALLOC %@ %s", _self, sel_getName(cmd));
        method_getImplementation(superDealloc)(_self, cmd);
    });
    class_addMethod(jsClass, @selector(dealloc), dealloc, method_getTypeEncoding(superDealloc));
    
    Method superRetain = class_getInstanceMethod(superClass, @selector(retain));
    IMP retain = imp_implementationWithBlock(^(id _self, SEL cmd){
        ILOG(@"RETAIN %@", jsClass);
        return method_getImplementation(superRetain)(_self, cmd);
    });
    class_addMethod(jsClass, @selector(retain), retain, method_getTypeEncoding(superRetain));
    
    return jsClass;
}

char *encodingToChar(int encoding){
    /**
     * \brief Reprents an invalid type (e.g., where no type is available).
     */

    if (!encoding) {
        RuntimeException(@"bad encoding!");
    }
    
    static char *types[200];
    //This is assigned for completeness, but throw an exception otherwise
    types[ObjCType_Invalid] =  NULL,
    
    /**
     * \brief A type whose specific kind is not exposed via this
     * interface.
     */
    types[ObjCType_Unexposed] =  "?",
   
    /* Builtin types */
    types[ObjCType_Void] = @encode(void),
    types[ObjCType_Bool] =  @encode(BOOL),
    types[ObjCType_Char_U] =  @encode(unsigned char),
    types[ObjCType_UChar] =  @encode(u_char),
    types[ObjCType_Char16] =  @encode(char),
    types[ObjCType_Char32] =  @encode(char),
    types[ObjCType_UShort] =  @encode(unsigned short),
    types[ObjCType_UInt] =  @encode(unsigned int),
    
    types[ObjCType_ULong] =  @encode(unsigned long),
    types[ObjCType_ULongLong] =  @encode(unsigned long long),
    types[ObjCType_UInt128] = @encode(unsigned int),
    types[ObjCType_Char_S] =  @encode(char),
    types[ObjCType_SChar] =  @encode(char *),
    types[ObjCType_WChar] =  @encode(wchar_t),
    types[ObjCType_Short] =  @encode(short),
    types[ObjCType_Int] =  @encode(int),
    types[ObjCType_Long] =  @encode(long),
    types[ObjCType_LongLong] =  @encode(long long),
    types[ObjCType_Int128] =  @encode(long long),
    types[ObjCType_Float] =  @encode(float),
    types[ObjCType_Double] =  @encode(double),
    types[ObjCType_LongDouble] =  @encode(long double),
    types[ObjCType_NullPtr] =  @encode(int),
//    types[ObjCType_Overload] =  @encode(),
//    types[ObjCType_Dependent] =  26,
    types[ObjCType_ObjCId] =  @encode(id),
    types[ObjCType_ObjCClass] =  @encode(Class),
    types[ObjCType_ObjCSel] =  @encode(SEL),
    types[ObjCType_FirstBuiltin] =  types[ObjCType_Void],
    types[ObjCType_LastBuiltin ] =  types[ObjCType_ObjCSel],
    
//    types[ObjCType_Complex] = 100,
    types[ObjCType_Pointer] = @encode(pointer_t),
    types[ObjCType_BlockPointer] =  @encode(pointer_t),
//    types[ObjCType_LValueReference] =  @encode(0),
//    types[ObjCType_RValueReference] =  104,
    types[ObjCType_Record] =  "?",
    types[ObjCType_Enum] =  "?",
    types[ObjCType_Typedef] =  "?",
//    types[ObjCType_ObjCInterface] =  @encode(@interface()),
    types[ObjCType_ObjCObjectPointer] =  @encode(id),
//    types[ObjCType_FunctionNoProto] =  ,
//    types[ObjCType_FunctionProto] =  111,
//    types[ObjCType_ConstantArray] =  112,
//    types[ObjCType_Vector] =  113,
//    types[ObjCType_IncompleteArray] =  114,
//    types[ObjCType_VariableArray] =  115,
//    types[ObjCType_DependentSizedArray] =  116,
    types[ObjCType_MemberPointer] =  @encode(pointer_t);
    return types[encoding];
}

size_t encodingToSize(int encoding){
    size_t sizes[200];
    sizes[ObjCType_Invalid] =  0,
    
    /**
     * \brief A type whose specific kind is not exposed via this
     * interface.
     */
    sizes[ObjCType_Unexposed] =  -1,
   
    /* Builtin types */
    sizes[ObjCType_Void] = sizeof(void),
    sizes[ObjCType_Bool] =  sizeof(BOOL),
    sizes[ObjCType_Char_U] =  sizeof(unsigned char),
    sizes[ObjCType_UChar] =  sizeof(u_char),
    sizes[ObjCType_Char16] =  sizeof(char),
    sizes[ObjCType_Char32] =  sizeof(char),
    sizes[ObjCType_UShort] =  sizeof(unsigned short),
    sizes[ObjCType_UInt] =  sizeof(unsigned int),
    
    sizes[ObjCType_ULong] =  sizeof(unsigned long),
    sizes[ObjCType_ULongLong] =  sizeof(unsigned long long),
    sizes[ObjCType_UInt128] = sizeof(unsigned int),
    sizes[ObjCType_Char_S] =  sizeof(char *),
    sizes[ObjCType_SChar] =  sizeof(char),
    sizes[ObjCType_WChar] =  sizeof(wchar_t),
    sizes[ObjCType_Short] =  sizeof(short),
    sizes[ObjCType_Int] =  sizeof(int),
    sizes[ObjCType_Long] =  sizeof(long),
    sizes[ObjCType_LongLong] =  sizeof(long long),
    sizes[ObjCType_Int128] =  sizeof(long long),
    sizes[ObjCType_Float] =  sizeof(float),
    sizes[ObjCType_Double] =  sizeof(double),
    sizes[ObjCType_LongDouble] =  sizeof(long double),
    sizes[ObjCType_NullPtr] =  sizeof(int),
//    sizes[ObjCType_Overload] =  sizeof(),
//    sizes[ObjCType_Dependent] =  26,
    sizes[ObjCType_ObjCId] =  sizeof(id),
    sizes[ObjCType_ObjCClass] =  sizeof(Class),
    sizes[ObjCType_ObjCSel] =  sizeof(SEL),
    sizes[ObjCType_FirstBuiltin] =  sizes[ObjCType_Void],
    sizes[ObjCType_LastBuiltin ] =  sizes[ObjCType_ObjCSel],
   
    
//    sizes[ObjCType_Complex] = 100,
    sizes[ObjCType_Pointer] = sizeof(pointer_t),
    sizes[ObjCType_BlockPointer] =  sizeof(pointer_t),
//    sizes[ObjCType_LValueReference] =  sizeof(<#a#>),
//    sizes[ObjCType_RValueReference] =  104,
//    sizes[ObjCType_Record] =  sizeof(stru),
//    sizes[ObjCType_Enum] =  106,
//    sizes[ObjCType_Typedef] =  sizeof,
//    sizes[ObjCType_ObjCInterface] =  sizeof(@interface()),
    sizes[ObjCType_ObjCObjectPointer] =  sizeof(id),
//    sizes[ObjCType_FunctionNoProto] =  ,
//    sizes[ObjCType_FunctionProto] =  111,
//    sizes[ObjCType_ConstantArray] =  112,
//    sizes[ObjCType_Vector] =  113,
//    sizes[ObjCType_IncompleteArray] =  114,
//    sizes[ObjCType_VariableArray] =  115,
//    sizes[ObjCType_DependentSizedArray] =  116,
    sizes[ObjCType_MemberPointer] =  sizeof(pointer_t);
    return sizes[encoding];
}
void *cujs_newJSObjectClass(void){
    NSString *name = [NSString stringWithFormat:@"JSOBJ_%d", (int)random()];
    Class superClass = [NSObject class];
    return cujs_newSubclass(superClass, name);
}

void *cujs_newSubclass(Class superClass, NSString *name){
    ILOG(@"-%s %@",__PRETTY_FUNCTION__, name);
    const char *className = name.cujs_cString;
    if (objc_getClass(className)) {
        RuntimeException(@"tried to override exising class %@", name);
    }
    
    Class jsClass = objc_allocateClassPair(superClass, className, 16);

    Method superBody = class_getInstanceMethod(superClass, @selector(_cujs_body:));
    IMP body = imp_implementationWithBlock(^(id _self, SEL cmd, ...){
        RuntimeException(@"object is not a function");
        return nil;
    });
    class_addMethod(jsClass, @selector(_cujs_body:), body, method_getTypeEncoding(superBody));
    
    const char *enc = method_getTypeEncoding(superBody);
    class_addMethod(jsClass, @selector(_cujs_body:), (IMP)body, enc);
    objc_registerClassPair(jsClass);
    
    //Override dealloc and retain for debugging
    Method superDealloc = class_getInstanceMethod(superClass, @selector(dealloc));
    IMP dealloc = imp_implementationWithBlock(^(id _self, SEL cmd){
        ILOG(@"DEALLOC %@ %s", _self, sel_getName(cmd));
        method_getImplementation(superDealloc)(_self, cmd);
    });
    class_addMethod(jsClass, @selector(dealloc), dealloc, method_getTypeEncoding(superDealloc));
    
    Method superRetain = class_getInstanceMethod(superClass, @selector(retain));
    IMP retain = imp_implementationWithBlock(^(id _self, SEL cmd){
        ILOG(@"RETAIN %@", jsClass);
        return method_getImplementation(superRetain)(_self, cmd);
    });
    
    class_addMethod(jsClass, @selector(retain), retain, method_getTypeEncoding(superRetain));
    return jsClass;
}

BOOL ptrIsClass(void *ptr){
    return malloc_size(ptr) && [(id)ptr class] == ptr;
}

BOOL ptrIsJSFunctionClass(void *ptr){
    return [objc_getAssociatedObject(ptr, JSFunctionTag) boolValue];
}

BOOL setterArgumentTypeIsKnownStruct(Method setter){
    struct objc_method_description *description = method_getDescription(setter);
    char *types = description->types;
    size_t sizeOfTypes = strlen(types);
  
    //Fixme: this could be improved
    //If the last value of encoding is '@' bail out quickly
    if (types[sizeOfTypes-1] == '@'){
        return NO;
    }

    //Save mallocs
    static char *nameBuffer;
    static dispatch_once_t onceToken;
    NSUInteger maxLen = 100;
    dispatch_once(&onceToken, ^{
        nameBuffer = malloc(sizeof(char) * maxLen);
    });
    
    int nArgs = method_getNumberOfArguments(setter);
    method_getArgumentType(setter, nArgs-1, nameBuffer, maxLen);

    if (!nameBuffer) {
        return NO;
    }
    return cujs_isStruct(nameBuffer);
}

void *cujs_assignProperty(id target,
                            const char *name,
                            id value) {
    ILOG(@"%s %p %s %p %@", __PRETTY_FUNCTION__, target, name, value, value);
    assert(target && "target required for property assignment");
    assert(name && "name required for property assignment");

    //get the method body from the class
    //and add it to our class
    BOOL targetIsClass = ptrIsClass(target);
    if (ptrIsJSFunctionClass(value)){
        Class targetClass;

        if ([target respondsToSelector:@selector(targetClass)]) {
            targetClass = [(id)target targetClass];
            ILOG(@"Prototype:");
        } else {
            targetClass =  object_getClass(target);
        }
        
        ILOG(@"add %@ method to class %@", targetIsClass ? @"class" : @"instance", (id)targetClass);
       
        Class valueClass = value;
        
        Method valueMethod = class_getInstanceMethod(valueClass, @selector(_cujs_body:));

        SEL newSelector = sel_getUid(name);
        class_replaceMethod(targetClass, newSelector,
                            method_getImplementation(valueMethod),
                            method_getTypeEncoding(valueMethod));
        ILOG(@"added method with selector %s", sel_getName(newSelector));
    } else {
        ILOG(@"add property");
        BOOL status = [target cujs_defineProperty:name];
        ILOG(@"defined property: %d with name %s:", status, name);
        char *setterName = setterNameFromPropertyName(name);
        SEL setterSelector = sel_getUid(setterName);
       
        //We need to use kvc to handle setters with structs as arguments
        BOOL isStructSetter = NO;
        if (!ptrIsClass(target)) {
            isStructSetter = setterArgumentTypeIsKnownStruct(class_getInstanceMethod([target class], setterSelector));
        }
        
        if (!isStructSetter) {
            objc_msgSend(target, setterSelector, [value retain]);
        } else {
            NSValue *structValue = [value structValue];
            [target setValue:structValue forKey:[NSString stringWithUTF8String:name]];
        }
    }
    
    return nil;
}


void *cujs_invoke(void *target, ...){
    ILOG(@"cujs_invoke(%p, ...)\n", target);

    if (!target) {
        RuntimeException(@"nil is not a function");
    }
  
    int argCt = 0;
    va_list args1;
    va_start(args1, target);
    for (NSString *arg = target; arg != nil; arg = va_arg(args1, id)){
        argCt++;
    }
    va_end(args1);
 
    BOOL targetIsClass = ptrIsClass(target);
    Class targetClass = nil;
    if (targetIsClass) {
        targetClass = target;
        target = cujs_GlobalScope;
        ILOG(@"global invocation %s \n", class_getName(targetClass));
    } else if ([(id)target respondsToSelector:@selector(_cujs_body:)]) {
        targetClass = object_getClass(target);
        ILOG(@"instance of %s\n", class_getName(targetClass));
    } else {
        RuntimeException(@"cujs_invoke requires _cujs_body:");
    }
  
    assert(targetClass && "cujs_invoke on missing class:" && target);
    
    SEL bodySel = @selector(_cujs_body:);
    va_list args;
    va_start(args, target);
    
    Method m = class_getInstanceMethod(targetClass, bodySel);
    IMP imp = method_getImplementation(m);
    
    id result;
    switch (argCt) {
        case 1: {
            result = imp(target, bodySel, nil);
            break;
        }
        case 2: {
            id arg1 = va_arg(args, id);
            result = imp(target, bodySel, arg1, nil);
            break;
        }
        case 3: {
            id arg1 = va_arg(args, id);
            id arg2 = va_arg(args, id);
            result = imp(target, bodySel, arg1, arg2, nil);
            break;
        }
        case 4: {
            id arg1 = va_arg(args, id);
            id arg2 = va_arg(args, id);
            id arg3 = va_arg(args, id);
            result = imp(target, bodySel, arg1, arg2, arg3, nil);
            break;
        }
        case 5: {
            id arg1 = va_arg(args, id);
            id arg2 = va_arg(args, id);
            id arg3 = va_arg(args, id);
            id arg4 = va_arg(args, id);
            result = imp(target, bodySel, arg1, arg2, arg3, arg4, nil);
            break;
        }
        default: {
            NSLog(@"warning failed %s %@ %s: (%d)", __PRETTY_FUNCTION__, target, sel_getName(bodySel), argCt);
            result = nil;
        }
            
    }
    va_end(args);
    return result;
}

@interface CUJSNaN : NSObject @end
@implementation CUJSNaN @end

@interface CUJSUndefined : NSObject @end
@implementation CUJSUndefined @end

id cujs_NaN;
id cujs_Undefined;
id cujs_GlobalScope;

__attribute__((constructor))
static void CUJSRuntimeInit(){
    cujs_NaN = [CUJSNaN class];
    cujs_Undefined = [CUJSUndefined class];
    cujs_GlobalScope = [NSObject new];
}

@implementation NSObject (CUJSOperators)

#define DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(SEL)\
- SEL (id) value { \
    BOOL sameClass = [self isKindOfClass:[value class]] ||  [value isKindOfClass:[self class]]; \
    return sameClass ? [self SEL value] : @0; \
}

DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_add:)
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_subtract:);
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_multiply:);
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_divide:);
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_mod:);
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_bitor:);
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_bitxor:);
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_bitand:);
// "<<"
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_shiftleft:);
// ">>"
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_shiftright:);
// ">>>"
DEF_CUJS_OPERATOR_RETURN_CLASS_OR_ZERO(cujs_shiftrightright:);

- cujs_increment {
    return cujs_NaN;
}

- cujs_decrement {
    return cujs_NaN;
}

- (bool)cujs_boolValue {
    return true;
}

@end

@implementation NSNumber (CUJSOperators)

- cujs_add:(id)value {
    return @([self doubleValue] + [value doubleValue]);
}

- cujs_subtract:(id)value {
    return @([self doubleValue] - [value doubleValue]);
}

- cujs_multiply:(id)value {
    return @([self doubleValue] * [value doubleValue]);
}

- cujs_divide:(id)value {
    return @([self doubleValue] / [value doubleValue]);
}

- cujs_mod:(id)value {
    return @([self intValue] % [value intValue]);
}

- cujs_bitor:(id)value {
    return @([self intValue] | [value intValue]);
}

- cujs_bitxor:(id)value {
    return @([self intValue] ^ [value intValue]);
}

- cujs_bitand:(id)value {
    return @([self intValue] & [value intValue]);
}

// "<<"
- cujs_shiftleft:(id)value {
    return @([self intValue] << [value intValue]);
}

// ">>"
- cujs_shiftright:(id)value {
    return @([self intValue] >> [value intValue]);
}

// ">>>"
- cujs_shiftrightright:(id)value {
    return @(([self intValue] >> [value intValue]) | 0);
}

- cujs_increment {
    if ([self objCType] == (const char *)'d') {
        return [NSNumber numberWithDouble:[self doubleValue] + 1.0];
    }
    
    return [NSNumber numberWithInt:[self doubleValue] + 1];
}

- cujs_decrement {
    if ([self objCType] == (const char *)'d') {
        return [NSNumber numberWithDouble:[self doubleValue] - 1.0];
    }
   
    return [NSNumber numberWithInt:[self doubleValue] - 1];
}

- (bool)cujs_boolValue {
    return !![self intValue];
}

@end

@implementation NSString (CUJSOperators)

- (bool)cujs_boolValue {
    return !![self length];
}

@end

@implementation NSObject (CUJSSubscripting)

- (void)cujs_addObject:(id)anObject {
    ILOG(@"%s", __PRETTY_FUNCTION__);
}

- (void)cujs_insertObject:(id)anObject atIndex:(id)index {
    ILOG(@"%s", __PRETTY_FUNCTION__);
}

- (void)cujs_removeLastObject {
    ILOG(@"%s", __PRETTY_FUNCTION__);
}

- (void)cujs_removeObjectAtIndex:(id)index {
    ILOG(@"%s", __PRETTY_FUNCTION__);
}

- (NSMutableDictionary *)_cujs_keyed_properties {
    static const char *KeyedPropertiesKey;
    NSMutableDictionary *keyedProperties = objc_getAssociatedObject(self, KeyedPropertiesKey);
    if (!keyedProperties) {
        keyedProperties = [NSMutableDictionary dictionary];
        objc_setAssociatedObject(self, KeyedPropertiesKey, keyedProperties, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return keyedProperties;
}

- (void)cujs_ss_setValue:(id)value forKey:(id)key {
    ILOG(@"%s %@[%@] = %@", __PRETTY_FUNCTION__, self, key, value);
    if ([key isKindOfClass:[NSString class]]) {
        char *setterName = setterNameFromPropertyName([key cujs_cString]);
        SEL setter = sel_getUid(setterName);
        free(setterName);
        
        if ([self respondsToSelector:setter]) {
            [self performSelector:setter withObject:value];
        } else {
            [self _cujs_keyed_properties][key] = value;
        }
        
    } else {
        [self cujs_ss_setValue:value forKey:[key stringValue]];
    }
}

- (id)cujs_ss_valueForKey:(id)index {
    ILOG(@"%s %@[%@]", __PRETTY_FUNCTION__, self, index);
    if ([index isKindOfClass:[NSString class]]) {
        const char *getterName = [index cujs_cString];
        SEL getter = sel_getUid(getterName);
        
        if ([self respondsToSelector:getter]) {
            return [self performSelector:getter];
        }
        
        return [self _cujs_keyed_properties][index];
    }

    //FIXME: this will infinitely recurse
    return [self cujs_ss_valueForKey:[index stringValue]];
}

@end

@implementation NSObject (CUJSDebug)

- (void)cujs_printMethods
{
    [self.class cujs_printMethods];
}

+ (void)cujs_printMethods
{
     // Iterate over the class and all superclasses
    Class currentClass = [self class];
    while (currentClass) {
        // Iterate over all instance methods for this class
        unsigned int methodCount;
        Method *methodList = class_copyMethodList(currentClass, &methodCount);
        unsigned int i = 0;
        for (; i < methodCount; i++) {
            NSLog(@"%@ - %@", [NSString stringWithCString:class_getName(currentClass) encoding:NSUTF8StringEncoding], [NSString stringWithCString:sel_getName(method_getName(methodList[i])) encoding:NSUTF8StringEncoding]);
        }
        
        free(methodList);
        currentClass = class_getSuperclass(currentClass);
    }   
}

@end

@implementation CUJSPrototype

@end

@implementation NSObject (CUJSStruct)

NSMutableSet *CUJSStructClasses();
NSMutableSet *CUJSStructs();

+ (NSDictionary *)fieldOffsets {
    return @{};
}

+ (NSDictionary *)fieldTypeNames{
    return @{};
}

+ (NSNumber *)structSize {
    return nil;
}

+ (NSString *)structEncoding {
    return nil;
}

+ (NSDictionary *)structFieldEncoding {
    return nil;
}

// convert the struct to the actual struct type and set it on the key
- (id)structValue {
    NSDictionary *offsets = [[self class] fieldOffsets] ;
    NSNumber *structSize = [[self class] structSize] ;
    NSString *structEncoding = [[self class] structEncoding];
    NSDictionary *fieldEncoding = [[self class] structFieldEncoding];
    NSDictionary *fieldTypeNames = [[self class] fieldTypeNames] ;
    
    if (!(offsets
        && structSize) &&
        (structEncoding
        && fieldEncoding)) {
        NSLog(@"warning - invoked -[NSObject structValue] on incomplete struct type");
        return nil;
    }
    
    __block char *structBytes = malloc(structSize.intValue);
    for(id key in offsets) {
        id obj = [offsets valueForKey:key];
        NSUInteger offset = [obj integerValue];
        NSString *fieldEncodingString = fieldEncoding[key];
        const char *fieldEncoding = fieldEncodingString.cujs_cString;

        assert(fieldEncoding && "Missing field encoding for string");
        id value = [[self valueForKey:key] retain];
        if (!strcmp("i", fieldEncoding)) {
            assert([self respondsToSelector:NSSelectorFromString(key)]);
            *(int *)(structBytes + offset) = [value intValue];
        } else if (!strcmp("f", fieldEncoding)) {
            *(float *)(structBytes + offset) = [value floatValue];
        } else if (!strcmp("d", fieldEncoding)) {
            *(double *)(structBytes + offset) = [value doubleValue];
        } else {
            NSString *typeName = fieldTypeNames[key];
            NSString *prefixedTypeNameString = cujs_prefixedStructClassNameWithString(typeName);
            BOOL isStruct = cujs_isStruct([typeName cujs_cString]) || [CUJSStructClasses() containsObject:prefixedTypeNameString];
            if (isStruct) {
                Class valueClass = objc_getClass([prefixedTypeNameString cujs_cString]);
                assert(valueClass);
                int valueSize = [[valueClass structSize] intValue];
                char *valueBytes = (char *)[value toStruct];
                memcpy(structBytes + offset, valueBytes, valueSize);
                free(valueBytes);
            } else {
                NSLog(@"waring unsupported struct member type (not struct) %@", fieldEncodingString);
                abort();
            }
        }
    }

    NSValue *structValue = [NSValue valueWithBytes:structBytes objCType:structEncoding.cujs_cString];
    free(structBytes);
    return structValue;
}

- (char *)toStruct {
    NSNumber *structSize = [[self class] structSize];
    assert(structSize && "missing required size");
    NSValue *structValue = [self structValue];
    assert(structValue);
    void *ptr = malloc(structSize.intValue);
    [structValue getValue:ptr];
    return ptr;
}

+ (BOOL)cujs_isStruct {
    return !![self structSize];
}

- (BOOL)cujs_isStruct {
    return [[self class] cujs_isStruct];
}

@end

NSMutableSet *CUJSStructClasses(){
    static id instance;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[NSMutableSet alloc] init];
    });
    return instance;
}

NSMutableSet *CUJSStructs(){
    static id instance;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[NSMutableSet alloc] init];
    });
    return instance;
}

extern void *cujs_defineStruct(const char *name, size_t size, CUJSStructField fields[]) {
    NSString *prefixedClassName = cujs_prefixedStructClassNameWithCString(name);
    Class existingClass = NSClassFromString(prefixedClassName);
    if (existingClass) {
        return existingClass;
    }

    //FIXME: this should really be for debug only
    //Check support
     for (int i = 0; i < size; i++){
        CUJSStructField field = fields[i];
        //assignn the encoding
         char *fieldEncoding = encodingToChar(field.encoding);
         if (!fieldEncoding) {
             NSLog(@"warning found unsupported struct %s violating field encoding %d", name, field.encoding);
             return NULL;
         }
         
         BOOL hasRestrictedTypeName = cujs_isRestrictedProperty(field.name);
         if (hasRestrictedTypeName) {
             NSLog(@"warning found unsupported struct %s violating field has restricted name %s", name, field.typeName);
             return NULL;
         }
     }
    
    //Declare a new subclass and add it to the struct classes
    NSString *nameString = [NSString stringWithUTF8String:name];
    Class structClass = cujs_newSubclass([NSObject class], prefixedClassName);
    [CUJSStructClasses() addObject:prefixedClassName];

    char *encodingBuffer = malloc(sizeof(char) * 1000);
    encodingBuffer[0] = '{';
    int nameOffset = 1;
    for (unsigned i = 0; i < strlen(name); i++) {
        encodingBuffer[nameOffset++] = name[i];
    }
    
    encodingBuffer[nameOffset] = '=';

    NSMutableDictionary *fieldOffsets = [NSMutableDictionary dictionary];
    NSMutableDictionary *fieldEncodigns = [NSMutableDictionary dictionary];
    NSMutableDictionary *fieldTypeNames = [NSMutableDictionary dictionary];
    
    size_t structSize = 0;
    for (int i = 0; i < size; i++){
        CUJSStructField field = fields[i];
        //assignn the encoding
        char *fieldEncoding = encodingToChar(field.encoding);

        NSString *typeNameString = [NSString stringWithUTF8String:field.typeName];
        NSString *fieldName = [NSString stringWithUTF8String:field.name];
        fieldOffsets[fieldName] = @(field.offset);
        fieldEncodigns[fieldName] = [NSString stringWithUTF8String:fieldEncoding];
        fieldTypeNames[fieldName] = typeNameString;
       
        ILOG(@"FieldName %s", field.name);
        ILOG(@"FieldTypeName %s", field.typeName);
        ILOG(@"FieldOffset %d", field.offset);
        ILOG(@"FieldEncoding %d", field.encoding);
        
        if (strcmp("?", fieldEncoding) == 0) {
            NSString *prefixedTypeNameString = cujs_prefixedStructClassNameWithString(typeNameString);
            if ([CUJSStructClasses() containsObject:prefixedTypeNameString]) {
                id fieldClass = objc_getClass([prefixedTypeNameString cujs_cString]);
                assert(fieldClass);
               
                nameOffset++;
                encodingBuffer[nameOffset] = '=';
                
                NSString *fieldEncodingActualString = [fieldClass structEncoding];
                if (!fieldEncodingActualString) {
//                    abort();
                    [CUJSStructClasses() removeObject:prefixedTypeNameString];
                    ILOG(@"warning - cannot create struct: %s missing typename string %@", name, typeNameString);
                    free(encodingBuffer);
                    return NULL;
                }
                
                const char *fieldEncodingActual = [fieldEncodingActualString cujs_cString];
                size_t fieldEncodingLen = strlen(fieldEncodingActual);
                for (unsigned j = 0; j < fieldEncodingLen; j++) {
                    encodingBuffer[nameOffset++] = fieldEncodingActual[j];
                }
                
                nameOffset--;
                size_t fieldSize = [[fieldClass structSize] intValue];
                structSize += fieldSize;
            } else {
//                abort();
                ILOG(@"warning - cannot create struct: %s missing typename string %@", name, typeNameString);
                [CUJSStructClasses() removeObject:prefixedTypeNameString];
                free(encodingBuffer);
                return NULL;
            }
        } else {
            nameOffset++;
            encodingBuffer[nameOffset] = fieldEncoding[0];
            
            structSize += encodingToSize(field.encoding);
        }
        
        cujs_defineProperty(structClass, field.name);
    }
    
    nameOffset++;
    encodingBuffer[nameOffset] = '}';
    
    nameOffset++;
    encodingBuffer[nameOffset] = '\0';
    cujs_registerStruct(encodingBuffer);
    NSString *encodingString = [NSString stringWithUTF8String:encodingBuffer];
    cujs_assignProperty(structClass,
                        "structType",
                        encodingString);
    
    cujs_assignProperty(structClass,
                        "fieldOffsets",
                        fieldOffsets);

    cujs_assignProperty(structClass,
                        "fieldTypeNames",
                        fieldTypeNames);
    
    cujs_assignProperty(structClass,
                        "structFieldEncoding",
                        fieldEncodigns);
   
    //Fixme - test leak on this number
    cujs_assignProperty(structClass, "structSize", [@(structSize) retain]);
    cujs_assignProperty(structClass, "structEncoding", encodingString);

    free(encodingBuffer);

    return structClass;
}

void cujs_registerStruct(const char *name) {
    [CUJSStructs() addObject:[NSString stringWithUTF8String:name]];
}

extern BOOL cujs_isStruct(const char *name) {
    return [CUJSStructs() containsObject:[NSString stringWithUTF8String:name]];
}

extern void *objc_Struct(NSString *nameString, ...) {
    NSLog(@"%s %@", __PRETTY_FUNCTION__, nameString);
//    const char *name = [nameString cujs_cString];
    Class structClass = objc_getClass([cujs_prefixedStructClassNameWithString(nameString) cujs_cString]);
    assert(structClass);
    id instance = [[structClass alloc] init];

    NSDictionary *offsets = [[[instance class] fieldOffsets] retain];
    NSNumber *structSize = [[[instance class] structSize] retain];
    NSString *structEncoding = [[[instance class] structEncoding] retain];
    NSDictionary *fieldEncoding = [[[instance class] structFieldEncoding] retain];
    NSDictionary *fieldTypeNames = [[instance class] fieldTypeNames];
    if (!((offsets
        && structSize) &&
        (structEncoding
        && fieldEncoding)
        && fieldTypeNames)) {
            abort();
            NSLog(@"warning - invoked -[NSObject structValue] on incomplete struct type");
            return nil;
    }

    
    va_list args;
    va_start(args, nameString);
    const char *structBytes = va_arg(args, const char *);
    va_end(args);

    [offsets enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
        NSInteger offset = [obj integerValue];
        NSString *fieldEncodingString = fieldEncoding[key];
        const char *fieldEncoding = fieldEncodingString.cujs_cString;

        assert(fieldEncoding && "Missing field encoding for string");
        if (!strcmp("i", fieldEncoding)) {
            assert([instance respondsToSelector:NSSelectorFromString(key)]);
            int value = *(int *)(structBytes + offset);
            cujs_assignProperty(instance, [key cujs_cString], [NSNumber numberWithInt:value]);
        } else if (!strcmp("f", fieldEncoding)) {
            float value = *(float *)(structBytes + offset);
            cujs_assignProperty(instance, [key cujs_cString], [NSNumber numberWithFloat:value]);
        } else if (!strcmp("d", fieldEncoding)) {
            double value = *(double *)(structBytes + offset);
            cujs_assignProperty(instance, [key cujs_cString], [NSNumber numberWithDouble:value]);
        } else if (!strcmp("?", fieldEncoding)) {
            //Recurse and set value to the result
            NSString *typeName = fieldTypeNames[key];
            BOOL isStruct = cujs_isStruct([typeName cujs_cString]) || [CUJSStructClasses() containsObject:cujs_prefixedStructClassNameWithString(typeName)];
            if (isStruct) {
                id structValue = objc_Struct(typeName, (structBytes + offset));
                cujs_assignProperty(instance, [key cujs_cString], structValue);
            } else {
                NSLog(@"waring unsupported struct member type (not struct) %@ %@", typeName, fieldEncodingString);
            }
            
        } else {
            NSLog(@"waring unsupported struct member type %@", fieldEncodingString);
        }
    }];

    return [instance autorelease];
}
