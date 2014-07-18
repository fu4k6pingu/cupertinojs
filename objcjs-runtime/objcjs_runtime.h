//
//  objcjs_runtime.h
//  objcjs-runtime
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

//TODO : make static!
@interface JSFunction : NSObject

@property (nonatomic, copy) JSFunction *parent;
@property (nonatomic, strong) NSMutableDictionary *ivars;

+ (void)setParent:(id)parent;
+ (id)parent;

- (id)body:(id)args;

- (void)defineProperty:(const char *)propertyName;

- (void)_objcjs_env_setValue:(id)value forKey:(NSString *)key;
- (id)_objcjs_env_valueForKey:(NSString *)key;

@end

typedef id JSFunctionBodyIMP (id _self, SEL sel, ...);

extern void *defineJSFunction(const char *name, JSFunctionBodyIMP body);

//dynamically dispatch to a pointer
// if target is a class class, a new instance of the class is created
// then invokes body:
//
// otherwise it simply invokes body:
extern void *objcjs_invoke(void *target, ...);