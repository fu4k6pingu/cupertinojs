//
//  objcjs_runtime.m
//  objcjs-runtime
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import "objcjs_runtime.h"
#import <objc/message.h>

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
    self.ivars = [[[NSMutableDictionary alloc] init] retain];
    return self;
}

- (id)body:(id)args {
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
    _ivars[[key copy]] = [value copy];
}

- (id)_objcjs_env_valueForKey:(NSString *)key {
    return _ivars[key];
}

@end
//an imp signature is id name(_strong id, SEL, ...);
void *defineJSFunction(const char *name, JSFunctionBodyIMP body){
    NSLog(@"%s %s %p",__PRETTY_FUNCTION__, name, body);
    Class superClass = [JSFunction class];
    Class jsClass = objc_allocateClassPair(superClass, name, 16);
   
    Method superBody = class_getInstanceMethod(superClass, @selector(body:));
    const char *enc = method_getTypeEncoding(superBody);
    assert(class_addMethod(jsClass, @selector(body:), (IMP)body, enc));
    objc_registerClassPair(jsClass);
    return jsClass;
}

void *objcjs_invoke(void *target, ...){
    Class targetClass;
    if (![(id)target isKindOfClass:[JSFunction class]]){
        targetClass = target;
        target = class_createInstance(target, 0);
    } else {
        targetClass = object_getClass(target);
    }
    
    SEL bodySel = @selector(body:);
    va_list args;
    va_start(args, target);
    
    Method m = class_getInstanceMethod(targetClass, @selector(body:));
    int nArgs = method_getNumberOfArguments(m);
    IMP imp = method_getImplementation(m);
    
    id result;
    switch (nArgs) {
        case 2: {
            result = imp(target, bodySel);
            break;
        }
        case 3: {
            id arg1 = va_arg(args, id);
            result = imp(target, bodySel, arg1);
            break;
        }
        case 4: {
            id arg1 = va_arg(args, id);
            id arg2 = va_arg(args, id);
            result = imp(target, bodySel, arg1, arg2);
            break;
        }
    }
    va_end(args);
    return result;
}
