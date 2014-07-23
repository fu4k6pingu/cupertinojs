//
//  objcjs_runtime.m
//  objcjs-runtime
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import "objcjs_runtime.h"
#import <objc/message.h>

#define RuntimeException(_FMT) \
    [NSException raise:@"OBJCJSRuntimeException" format:_FMT]


@implementation JSFunction

static NSMutableDictionary *Parents = NULL;

+ (void)setParent:(id)parent {
    if (!Parents){
        Parents = [[NSMutableDictionary alloc] init];
    }

    Parents[NSStringFromClass(self.class)] = [parent retain];
}

+ (id)parent {
    return Parents[NSStringFromClass(self.class)];
}

- (id)init {
    self = [super init];
    self.environment = [[[NSMutableDictionary alloc] init] retain];
    return self;
}

- (id)parent {
    return Parents[NSStringFromClass(self.class)];
}

- (id)body:(id)args, ... {
    return @"JSFUNCTION-DEFAULT-BODY-RETURN";
}

- (NSNumber *)length {
    return @0;
}

- (NSString *)description {
    return @"JSFUNCTION";
}

+ (instancetype)new{
    return [[super new]retain];
}

- (void)defineProperty:(const char *)propertyName {
    Class selfClass = [self class];
    
    IMP getter = imp_implementationWithBlock(^(id _self, SEL cmd){
        id value = objc_getAssociatedObject(_self, propertyName);
        return [value retain];
    });
    
    size_t nameLen = strlen(propertyName);
    char *setterName = malloc((sizeof(char) * nameLen) + 5);
  
    memcpy(setterName, "set", 4);
    setterName[3] = toupper(propertyName[0]);
   
    for (int i = 1; i < nameLen; i++) {
        setterName[i + 3] = propertyName[i];
    }
    
    setterName[nameLen + 3] = ':';
    setterName[nameLen + 4] = '\0';
    
    IMP setter = imp_implementationWithBlock(^(JSFunction *_self, SEL cmd, id value){
        objc_setAssociatedObject(_self, propertyName, value, OBJC_ASSOCIATION_COPY);
    });
    
    SEL gSel = sel_getUid(propertyName);
    class_addMethod(selfClass, gSel, getter, "@@:");
    class_addMethod(selfClass, sel_getUid(setterName), setter, "v@:@");

    free(setterName);
}

- (void)_objcjs_env_setValue:(id)value forKey:(NSString *)key {
    if (!key) {
        if (![[_environment allKeys] containsObject:key]) {
            [self.parent _objcjs_env_setValue:value forKey:key];
        } else {
            [_environment setNilValueForKey:[key copy]];
        }
    } else {
        if (![[_environment allKeys] containsObject:key]) {
            [self.parent _objcjs_env_setValue:value forKey:key];
        } else {
            _environment[[key copy]] = [value copy];
        }   
    }
}

- (void)_objcjs_env_setValue:(id)value declareKey:(NSString *)key {
    _environment[[key copy]] = [value copy];
}

- (id)_objcjs_env_valueForKey:(NSString *)key {
    id value = _environment[key];
    return value ?: [self.parent _objcjs_env_valueForKey:key];
}

@end


SEL BodySelector(NSInteger nArgs){
    static char * BodySelectors[] = {
        "body",
        "body:",
        "body::",
        "body:::",
        "body::::",
    };
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        for (int i = 0; i < 5; i++){
            sel_registerName(BodySelectors[i]);
        }
    });

    SEL selector = sel_getUid(BodySelectors[nArgs]);
    return selector;
}


//an imp signature is id name(_strong id, SEL, ...);
void *defineJSFunction(const char *name,
                       JSFunctionBodyIMP body){
    NSLog(@"%s %s %p",__PRETTY_FUNCTION__, name, body);
    Class superClass = [JSFunction class];
    Class jsClass = objc_allocateClassPair(superClass, name, 16);

    SEL bodySelector = @selector(body:);
    Method superBody = class_getInstanceMethod(superClass, bodySelector);
    const char *enc = method_getTypeEncoding(superBody);
    assert(class_addMethod(jsClass, bodySelector, (IMP)body, enc));
    objc_registerClassPair(jsClass);
    return jsClass;
}

void *objcjs_invoke(void *target, ...){
    printf("objcjs_invoke(%p, ...)\n", target);

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
    
    Class targetClass;
    if (![(id)target isKindOfClass:[JSFunction class]]){
        targetClass = target;
// TODO: handle this retain count craze!
//        target = class_createInstance(target, 0);
        target = [(id)target new];
        printf("new instance of %s \n", class_getName(targetClass));
    } else {
        targetClass = object_getClass(target);
        printf("instance of %s\n", class_getName(targetClass));
    }
  
    assert(targetClass && "objcjs_invoke on missing class:" && target);
    
    SEL bodySel = @selector(body:);
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

@implementation NSNumber (ObjcJSOperators)

- (instancetype)objcjs_add:(id)value {
    return @([self doubleValue] + [value doubleValue]);
}

- (instancetype)objcjs_subtract:(id)value {
    return @([self doubleValue] - [value doubleValue]);
}

- (instancetype)objcjs_multiply:(id)value {
    return @([self doubleValue] * [value doubleValue]);
}

- (instancetype)objcjs_divide:(id)value {
    return @([self doubleValue] / [value doubleValue]);
}

- (instancetype)objcjs_mod:(id)value {
    return @([self intValue] % [value intValue]);
}

- (instancetype)objcjs_increment {
    NSNumber **s =&self;
    const char d = 'd';
    if (*[*s objCType] == d) {
        double result = [self doubleValue] + 1.0;
        *s = [NSNumber numberWithDouble:result];
    } else {
        *s = [NSNumber numberWithInt:[self doubleValue] + 1];
    }
    
    return *s;
}

- (instancetype)objcjs_decrement {
    NSNumber **s =&self;
    const char d = 'd';
    if (*[*s objCType] == d) {
        double result = [self doubleValue] - 1.0;
        *s = [NSNumber numberWithDouble:result];
    } else {
        *s = [NSNumber numberWithInt:[self doubleValue] - 1];
    }
    
    return *s;
}

- (bool)objcjs_boolValue {
    return !![self intValue];
}

@end

@implementation NSString (ObjcJSOperators)

- (bool)objcjs_boolValue {
    return !![self length];
}

@end
