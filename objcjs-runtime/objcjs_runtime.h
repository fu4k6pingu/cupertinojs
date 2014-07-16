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

- (id)body:(id)args;

@end

typedef id __strong *JSFunctionBodyIMP (id _self, SEL sel, id arg);

extern void *defineJSFunction(const char *name, JSFunctionBodyIMP body);
