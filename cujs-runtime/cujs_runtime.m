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
#define CUJSRuntimeDEBUG 1
#define ILOG(A, ...){if(CUJSRuntimeDEBUG){ NSLog(A,##__VA_ARGS__), printf("\n"); }}


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
    
    if ([selfClass respondsToSelector:getterSelector] &&
        [selfClass respondsToSelector:setterSelector]) {
        ILOG(@"%s - already defined", __PRETTY_FUNCTION__);
        return NO;
    }
    
    ILOG(@"%s: %s", __PRETTY_FUNCTION__, propertyName);
    
    IMP getter = imp_implementationWithBlock(^(id _self, SEL cmd){
        id propertyNameString = [NSString stringWithCString:propertyName encoding:NSUTF8StringEncoding];
        id value;
        if ([[[_self _cujs_keyed_properties] allKeys] containsObject:propertyNameString]) {
            value = [_self _cujs_keyed_properties][propertyNameString];
        } else {
            value = objc_getAssociatedObject(_self, propertyName);
        }
        
        ILOG(@"ACCESS VALUE FOR KEY %@ <%p>, %s - %s: %@", _self, _self, __PRETTY_FUNCTION__, propertyName, value);
        return value;
    });
    
    IMP setter = imp_implementationWithBlock(^(id _self, SEL cmd, id value){
        ILOG(@"SET VALUE %@ <%p>, %s - %@", _self, _self, __PRETTY_FUNCTION__, value);
        id propertyNameString = [NSString stringWithCString:propertyName encoding:NSUTF8StringEncoding];
        if ([[[_self _cujs_keyed_properties] allKeys] containsObject:propertyNameString]) {
            [_self _cujs_keyed_properties][propertyNameString] = value;
        } else {
            id _value = objc_getAssociatedObject(_self, propertyName);
            if (value != _value) {
                [_value release];
                objc_setAssociatedObject(_self, propertyName, [value retain], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
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

void *cujs_newJSObjectClass(void){
    NSString *name = [NSString stringWithFormat:@"JSOBJ_%ld", random()];
    Class superClass = [NSObject class];
    return cujs_newSubclass(superClass, name);
}

void *cujs_newSubclass(Class superClass, NSString *name){
    ILOG(@"-%s %@",__PRETTY_FUNCTION__, name);
    const char *className = name.cString;
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

void *cujs_assignProperty(id target,
                            const char *name,
                            id value) {
    ILOG(@"%s %p %p %p %@", __PRETTY_FUNCTION__, target, name, value, value);
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
    } else  {
        ILOG(@"add property");
        BOOL status = [target cujs_defineProperty:name];
        ILOG(@"defined property: %d", status);
        char *setterName = setterNameFromPropertyName(name);
        [target performSelector:sel_getUid(setterName) withObject:value];
        ILOG(@"added property with name %s value %@", name, value);
        ILOG(@"accessor returned %@", [target performSelector:sel_getUid(name)]);
        free(setterName);
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
        char *setterName = setterNameFromPropertyName([key cString]);
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
        char *getterName = [index cString];
        SEL getter = sel_getUid(getterName);
        
        if ([self respondsToSelector:getter]) {
            return [self performSelector:getter];
        }
        
        return [self _cujs_keyed_properties][index];
    }

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
