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

- (id)body:(id)args;

- (void)defineProperty:(const char *)propertyName;

@property (nonatomic, copy) NSString *identifier;

+ (void)setParent:(id)parent;
+ (id)parent ;

@end

typedef id __strong *JSFunctionBodyIMP (id _self, SEL sel, id arg);

extern void *defineJSFunction(const char *name, JSFunctionBodyIMP body);
