//
//  objcjs_runtime.h
//  objcjs-runtime
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

@interface NSObject (ObjCJSFunction)

- (NSMutableDictionary *)_objcjs_environment;

+ (void)_objcjs_setParent:(id)parent;
+ (id)_objcjs_parent;
+ (BOOL)objcjs_defineProperty:(const char *)propertyName;

- (id)_objcjs_parent;

- (id)_objcjs_body:(id)args,...;

- (BOOL)objcjs_defineProperty:(const char *)propertyName;

- (void)_objcjs_env_setValue:(id)value forKey:(NSString *)key;
- (void)_objcjs_env_setValue:(id)value declareKey:(NSString *)key;
- (id)_objcjs_env_valueForKey:(NSString *)key;

@end

#pragma mark - Public

typedef id JSFunctionBodyIMP (id instance, SEL cmd, id arg1,...);

extern void *objcjs_defineJSFunction(const char *name,
                              JSFunctionBodyIMP body);

//Value can be a class, or variable pointer
extern void *objcjs_assignProperty(id instance,
                                   const char *name,
                                   id value);

// dynamically dispatch an invocation of a pointer with arguments
//
// if target is a class class, a new instance of the class is created
// then invokes body:
//
// otherwise it simply invokes body:
extern void *objcjs_invoke(void *target, ...);

extern id objcjs_NaN;
extern id objcjs_Undefined;
extern id objcjs_GlobalScope;

@interface NSObject (ObjCJSOperators)

- objcjs_add:(id)value;
- objcjs_subtract:(id)value;
- objcjs_multiply:(id)value;
- objcjs_divide:(id)value;

- objcjs_mod:(id)value;

- objcjs_bitor:(id)value;
- objcjs_bitxor:(id)value;
- objcjs_bitand:(id)value;
// "<<"
- objcjs_shiftleft:(id)value;
// ">>"
- objcjs_shiftright:(id)value;
// ">>>"
- objcjs_shiftrightright:(id)value;
- objcjs_increment;
- objcjs_decrement;

- (bool)objcjs_boolValue;

@end

@interface NSNumber (ObjCJSOperators)

// Bool value is semantically equal to int value in JS land
// so comparing a NSDoubleNumber that is 0.0 will be incorrect in JS
// and zero is false
- (bool)objcjs_boolValue;

@end


@interface NSString (ObjCJSOperators)

// a string that has characters is true
- (bool)objcjs_boolValue;

@end
