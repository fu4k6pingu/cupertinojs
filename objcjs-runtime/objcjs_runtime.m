//
//  objcjs_runtime.m
//  objcjs-runtime
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import "objcjs_runtime.h"

@implementation JSFunction

- (id)body:(id)args {
//    NSLog(@"%s",__PRETTY_FUNCTION__);
    return @"JSFunction-default-return";
}

- (NSNumber *)length {
    return @0;
}

- (NSString *)description {
    return @"JSFunc";
}

+ (instancetype)new{
    return [[super new]retain];
}

@end

//an imp signature is id name(_strong id, SEL, ...);
void *defineJSFunction(const char *name, JSFunctionBodyIMP body){
    NSLog(@"%s %s %p",__PRETTY_FUNCTION__, name, body);
    Class superClass = [JSFunction class];
    Class jsClass = objc_allocateClassPair(superClass, name, 16);
   
    Method superBody = class_getInstanceMethod(superClass, @selector(body:));
    IMP impl = imp_implementationWithBlock(^(id arg, SEL cmd, ...){
//        NSLog(@"Added imp: %s %s", __PRETTY_FUNCTION__, sel_getName(cmd));
//        va_list args;
//        va_start(args, arg);
//        return body((__bridge id)arg, cmd, @"hello");
        return @"hello";
//        va_end(args);
    });
    const char *enc = method_getTypeEncoding(superBody);
//    bool status = class_addMethod(jsClass, @selector(body:), body, enc);
    bool status = class_addMethod(jsClass, @selector(body:), impl, enc);
    objc_registerClassPair(jsClass);
    return NULL;
}