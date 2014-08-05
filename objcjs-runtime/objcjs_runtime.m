//
//  objcjs_runtime.m
//  objcjs-runtime
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import "objcjs_runtime.h"
#import <objc/message.h>
#import <malloc/malloc.h>

#define RuntimeException(_FMT, ...) \
    [NSException raise:@"OBJCJSRuntimeException" format:_FMT, ##__VA_ARGS__]

#define ObjCJSRuntimeDEBUG 1
#define ILOG(A, ...){if(ObjCJSRuntimeDEBUG){ NSLog(A,##__VA_ARGS__), printf("\n"); }}


@implementation NSObject (ObjCJSFunction)

static char * ObjCJSEnvironmentKey;

+ (id)objcjs_new {
    return [[self new] autorelease];
}

- (NSMutableDictionary *)_objcjs_environment {
    NSMutableDictionary *environment = objc_getAssociatedObject(self, ObjCJSEnvironmentKey);
    if (!environment) {
        environment = [NSMutableDictionary dictionary];
        objc_setAssociatedObject(self, ObjCJSEnvironmentKey, environment, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return environment;
}

static NSMutableDictionary *ObjCJSParents = NULL;

+ (void)_objcjs_setParent:(id)parent {
    if (!ObjCJSParents){
        ObjCJSParents = [[NSMutableDictionary alloc] init];
    }

    ObjCJSParents[NSStringFromClass(self.class)] = parent;
}

+ (id)_objcjs_parent {
    return ObjCJSParents[NSStringFromClass(self.class)];
}

+ (BOOL)objcjs_defineProperty:(const char *)propertyName {
    return objcjs_defineProperty(object_getClass(self) , propertyName);
}

- (id)_objcjs_parent {
    return ObjCJSParents[NSStringFromClass(self.class)];
}

//TODO : undefined!
- (id)_objcjs_body:(id)args, ... {
    return @"JSFUNCTION-DEFAULT-BODY-RETURN";
}

//TODO : safely swizzle this on
- (NSNumber *)length {
    return @0;
}

//FIXME :
BOOL objcjs_isRestrictedProperty(const char *propertyName){
    if (strcmp("version", propertyName)) {
        return NO;
    }
    return YES;
}

BOOL objcjs_defineProperty(Class selfClass, const char *propertyName){
    if (objcjs_isRestrictedProperty(propertyName)) {
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
        if ([[[_self _objcjs_keyed_properties] allKeys] containsObject:propertyNameString]) {
            value = [_self _objcjs_keyed_properties][propertyNameString];
        } else {
            value = objc_getAssociatedObject(_self, propertyName);
        }
        
        ILOG(@"ACCESS VALUE FOR KEY %@ <%p>, %s - %s: %@", _self, _self, __PRETTY_FUNCTION__, propertyName, value);
        return value;
    });
    
    IMP setter = imp_implementationWithBlock(^(id _self, SEL cmd, id value){
        ILOG(@"SET VALUE %@ <%p>, %s - %@", _self, _self, __PRETTY_FUNCTION__, value);
        id propertyNameString = [NSString stringWithCString:propertyName encoding:NSUTF8StringEncoding];
        if ([[[_self _objcjs_keyed_properties] allKeys] containsObject:propertyNameString]) {
            [_self _objcjs_keyed_properties][propertyNameString] = value;
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

- (BOOL)objcjs_defineProperty:(const char *)propertyName {
    return objcjs_defineProperty([self class], propertyName);
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

- (void)_objcjs_env_setValue:(id)value forKey:(NSString *)key {
    NSMutableDictionary *environment = [self _objcjs_environment];
    if (!value) {
        if (![[environment allKeys] containsObject:key]) {
            [[self _objcjs_parent] _objcjs_env_setValue:[NSNull null] forKey:key];
        } else {
            environment[[key copy]] = [NSNull null];
        }
    } else {
        if (![[environment allKeys] containsObject:key]) {
            [[self _objcjs_parent] _objcjs_env_setValue:value forKey:key];
        } else {
            environment[[key copy]] = value;
        }   
    }
}

- (void)_objcjs_env_setValue:(id)value declareKey:(NSString *)key {
    NSMutableDictionary *environment = [self _objcjs_environment];
    if (!value) {
        environment[[key copy]] = [NSNull null];
    } else {
        environment[[key copy]] = value;
    }
}

- (id)_objcjs_env_valueForKey:(NSString *)key {
    NSMutableDictionary *environment = [self _objcjs_environment];
    id value = environment[key];
    return value && ![value isKindOfClass:[NSNull class]] ? value : [[self _objcjs_parent] _objcjs_env_valueForKey:key];
}

@end

@implementation NSObject (ObjCJSExtend)

+ (id)extend:(NSString *)name {
    return objcjs_newSubclass(self.class, name);
}

@end

const char *JSFunctionTag = "JSFunctionTag";

//an imp signature is id name(_strong id, SEL, ...);
void *objcjs_defineJSFunction(const char *name,
                       JSFunctionBodyIMP body){
    ILOG(@"-%s %s %p",__PRETTY_FUNCTION__, name, body);
    Class superClass = [NSObject class];
    Class jsClass = objc_allocateClassPair(superClass, name, 16);

    SEL bodySelector = @selector(_objcjs_body:);
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

void *objcjs_newJSObjectClass(void){
    NSTimeInterval time = [[NSDate date] timeIntervalSince1970];
    NSString *name = [NSString stringWithFormat:@"JSOBJ_%f", time];
    Class superClass = [NSObject class];
    return objcjs_newSubclass(superClass, name);
}

void *objcjs_newSubclass(Class superClass, NSString *name){
    ILOG(@"-%s %@",__PRETTY_FUNCTION__, name);
    const char *className = name.cString;
    if (objc_getClass(className)) {
        RuntimeException(@"tried to override exising class %@", name);
    }
    
    Class jsClass = objc_allocateClassPair(superClass, className, 16);
    
    Method superBody = class_getInstanceMethod(superClass, @selector(_objcjs_body:));
    IMP body = imp_implementationWithBlock(^(id _self, SEL cmd, ...){
        RuntimeException(@"object is not a function");
        return nil;
    });
    class_addMethod(jsClass, @selector(_objcjs_body:), body, method_getTypeEncoding(superBody));
    
    const char *enc = method_getTypeEncoding(superBody);
    class_addMethod(jsClass, @selector(_objcjs_body:), (IMP)body, enc);
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
    return [(id)ptr class] == ptr;
}

BOOL ptrIsJSFunctionClass(void *ptr){
    return [objc_getAssociatedObject(ptr, JSFunctionTag) boolValue];
}

void *objcjs_assignProperty(id target,
                            const char *name,
                            id value) {
    ILOG(@"%s %p %p %p %@", __PRETTY_FUNCTION__, target, name, value, [value class]);

    assert(target && "target required for property assignment");
    assert(name && "name required for property assignment");
    assert(value && "value required for property assignment");
    

    //get the method body from the class
    //and add it to our class
    BOOL targetIsClass = ptrIsClass(target);
    if (ptrIsJSFunctionClass(value)){
        ILOG(@"add %@  method", targetIsClass ? @"class" : @"instance");
        Class targetClass = object_getClass(target);
        
        Class valueClass = value;
        
        Method valueMethod = class_getInstanceMethod(valueClass, @selector(_objcjs_body:));

        size_t nameLen = strlen(name);
        char *methodName = malloc((sizeof(char) * nameLen) + 2);
       
        memcpy(methodName, name, nameLen);
        methodName[nameLen] = ':';
        methodName[nameLen+1] = '\0';
       
        SEL newSelector = sel_getUid(methodName);
        class_replaceMethod(targetClass, newSelector,
                            method_getImplementation(valueMethod),
                            method_getTypeEncoding(valueMethod));
        ILOG(@"added method with selector %s", sel_getName(newSelector));
    } else  {
        ILOG(@"add property");
        BOOL status = [target objcjs_defineProperty:name];
        ILOG(@"defined property: %d", status);
        char *setterName = setterNameFromPropertyName(name);
        [target performSelector:sel_getUid(setterName) withObject:value];
        ILOG(@"added property with name %s value %@", name, value);
        ILOG(@"accessor returned %@", [target performSelector:sel_getUid(name)]);
        free(setterName);
    }
    
    return nil;
}


void *objcjs_invoke(void *target, ...){
    ILOG(@"objcjs_invoke(%p, ...)\n", target);

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
        target = objcjs_GlobalScope;
        ILOG(@"global invocation %s \n", class_getName(targetClass));
    } else if ([(id)target respondsToSelector:@selector(_objcjs_body:)]) {
        targetClass = object_getClass(target);
        ILOG(@"instance of %s\n", class_getName(targetClass));
    } else {
        RuntimeException(@"objcjs_invoke requires _objcjs_body:");
    }
  
    assert(targetClass && "objcjs_invoke on missing class:" && target);
    
    SEL bodySel = @selector(_objcjs_body:);
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

@interface ObjCJSNaN : NSObject @end
@implementation ObjCJSNaN @end

@interface ObjCJSUndefined : NSObject @end
@implementation ObjCJSUndefined @end

id objcjs_NaN;
id objcjs_Undefined;
id objcjs_GlobalScope;

__attribute__((constructor))
static void ObjCJSRuntimeInit(){
    objcjs_NaN = [ObjCJSNaN class];
    objcjs_Undefined = [ObjCJSUndefined class];
    objcjs_GlobalScope = [NSObject new];
}

@implementation NSObject (ObjCJSOperators)

#define DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(SEL)\
- SEL (id) value { \
    BOOL sameClass = [self isKindOfClass:[value class]] ||  [value isKindOfClass:[self class]]; \
    return sameClass ? [self SEL value] : @0; \
}

DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_add:)
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_subtract:);
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_multiply:);
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_divide:);
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_mod:);
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_bitor:);
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_bitxor:);
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_bitand:);
// "<<"
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_shiftleft:);
// ">>"
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_shiftright:);
// ">>>"
DEF_OBJCJS_OPERATOR_RETURN_CLASS_OR_ZERO(objcjs_shiftrightright:);

- objcjs_increment {
    return objcjs_NaN;
}

- objcjs_decrement {
    return objcjs_NaN;
}

- (bool)objcjs_boolValue {
    return true;
}

@end

@implementation NSNumber (ObjCJSOperators)

- objcjs_add:(id)value {
    return @([self doubleValue] + [value doubleValue]);
}

- objcjs_subtract:(id)value {
    return @([self doubleValue] - [value doubleValue]);
}

- objcjs_multiply:(id)value {
    return @([self doubleValue] * [value doubleValue]);
}

- objcjs_divide:(id)value {
    return @([self doubleValue] / [value doubleValue]);
}

- objcjs_mod:(id)value {
    return @([self intValue] % [value intValue]);
}

- objcjs_bitor:(id)value {
    return @([self intValue] | [value intValue]);
}

- objcjs_bitxor:(id)value {
    return @([self intValue] ^ [value intValue]);
}

- objcjs_bitand:(id)value {
    return @([self intValue] & [value intValue]);
}

// "<<"
- objcjs_shiftleft:(id)value {
    return @([self intValue] << [value intValue]);
}

// ">>"
- objcjs_shiftright:(id)value {
    return @([self intValue] >> [value intValue]);
}

// ">>>"
- objcjs_shiftrightright:(id)value {
    return @(([self intValue] >> [value intValue]) | 0);
}

- objcjs_increment {
    if ([self objCType] == (const char *)'d') {
        return [NSNumber numberWithDouble:[self doubleValue] + 1.0];
    }
    
    return [NSNumber numberWithInt:[self doubleValue] + 1];
}

- objcjs_decrement {
    if ([self objCType] == (const char *)'d') {
        return [NSNumber numberWithDouble:[self doubleValue] - 1.0];
    }
   
    return [NSNumber numberWithInt:[self doubleValue] - 1];
}

- (bool)objcjs_boolValue {
    return !![self intValue];
}

@end

@implementation NSString (ObjCJSOperators)

- (bool)objcjs_boolValue {
    return !![self length];
}

@end

@implementation NSObject (ObjCJSSubscripting)

- (void)objcjs_addObject:(id)anObject {
    ILOG(@"%s", __PRETTY_FUNCTION__);
}

- (void)objcjs_insertObject:(id)anObject atIndex:(id)index {
    ILOG(@"%s", __PRETTY_FUNCTION__);
}

- (void)objcjs_removeLastObject {
    ILOG(@"%s", __PRETTY_FUNCTION__);
}

- (void)objcjs_removeObjectAtIndex:(id)index {
    ILOG(@"%s", __PRETTY_FUNCTION__);
}

- (NSMutableDictionary *)_objcjs_keyed_properties {
    static const char *KeyedPropertiesKey;
    NSMutableDictionary *keyedProperties = objc_getAssociatedObject(self, KeyedPropertiesKey);
    if (!keyedProperties) {
        keyedProperties = [NSMutableDictionary dictionary];
        objc_setAssociatedObject(self, KeyedPropertiesKey, keyedProperties, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return keyedProperties;
}

- (void)objcjs_replaceObjectAtIndex:(id)index withObject:(id)anObject {
    ILOG(@"%s %@[%@] = %@", __PRETTY_FUNCTION__, self, index, anObject);
    if ([index isKindOfClass:[NSString class]]) {
        char *setterName = setterNameFromPropertyName([index cString]);
        SEL setter = sel_getUid(setterName);
        free(setterName);
        
        if ([self respondsToSelector:setter]) {
            [self performSelector:setter withObject:anObject];
        } else {
            [self _objcjs_keyed_properties][index] = anObject;
        }
        
    } else {
        [self objcjs_replaceObjectAtIndex:[index stringValue] withObject:anObject];
    }
}

- (id)objcjs_objectAtIndex:(id)index {
    ILOG(@"%s %@[%@]", __PRETTY_FUNCTION__, self, index);
    if ([index isKindOfClass:[NSString class]]) {
        char *getterName = [index cString];
        SEL getter = sel_getUid(getterName);
        
        if ([self respondsToSelector:getter]) {
            return [self performSelector:getter];
        }
        
        return [self _objcjs_keyed_properties][index];
    }

    return [self objcjs_objectAtIndex:[index stringValue]];
}

@end

