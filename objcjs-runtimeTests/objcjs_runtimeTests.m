//
//  objcjs_runtimeTests.m
//  objcjs-runtimeTests
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "objcjs_runtime.h"

@interface objcjs_runtimeTests : XCTestCase

@end

@implementation objcjs_runtimeTests

static BOOL calledBody;

- (void)setUp
{
    [super setUp];
    calledBody = NO;
}

- (void)tearDown
{
    [super tearDown];
}

id impl1(id firstObject,
         SEL selector,
         id firstArgument, ...){
    calledBody = YES;

    id instance = firstObject;
    NSLog(@"%@", instance);
    NSLog(@"%s", sel_getName(selector));
    NSLog(@"%@", firstArgument);
    return firstArgument;
}

id impl2(id firstObject,
         SEL selector,
         id firstArgument,
         id secondArgument,
         ...){
    calledBody = YES;
    
    id instance = firstObject;
    NSLog(@"%@", instance);
    NSLog(@"%s", sel_getName(selector));
    NSLog(@"%@", firstArgument);
    NSLog(@"%@", secondArgument);
    return secondArgument;
}

id impl3(id instance,
         SEL selector,
         id firstArg, ...){
    calledBody = YES;
    NSLog(@"%@", firstArg);

    va_list argumentList;
    va_start(argumentList, firstArg);

    NSMutableArray *arguments = [NSMutableArray array];

    for (id arg = firstArg; arg != nil; arg = va_arg(argumentList, id)) {
        [arguments addObject:arg];
    }

    va_end(argumentList);
    return arguments;
}

- (void)testItCanCreateASubclass
{
    defineJSFunction("subclass-1", impl1);
    Class aClass = objc_getClass("subclass-1");
    JSFunction *f = [aClass new];
    id result = [f body:@"An Arg"];
    
    XCTAssertTrue(f, @"It creates a subclass");
    XCTAssertTrue(calledBody, @"It creates a subclass");
   
    //TODO : test the impl of body
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

- (void)testItCanDefineAPropety{
    const char *name = "name";
    JSFunction *f = [JSFunction new];
    [f defineProperty:name];
    XCTAssertTrue([f respondsToSelector:NSSelectorFromString(@"name")], @"It adds a getter");
    XCTAssertTrue([f respondsToSelector:NSSelectorFromString(@"setName:")], @"It adds a setter");
    
    [f performSelector:NSSelectorFromString(@"setName:") withObject:@3];
    id nameValue = [f performSelector:NSSelectorFromString(@"name")];
    XCTAssertEqual(nameValue, @3, @"It can set the value");
}

- (void)testItCanSetAParent {
    JSFunction *parent = [JSFunction new];
    defineJSFunction("child", impl1);
    Class childClass = objc_getClass("child");
    [childClass setParent:parent];
    XCTAssertEqual([childClass parent], parent, @"It has a parent");
}

- (void)testItCanInvokeAJSFunctionInstance {
    defineJSFunction("subclass-2", impl1);
    Class aClass = objc_getClass("subclass-1");
    JSFunction *f = [aClass new];
    id result = objcjs_invoke(f, CFRetain(@"An Arg"));
    
    XCTAssertTrue(calledBody, @"It invokes body:");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunction {
    defineJSFunction("subclass-3", impl1);
    Class aClass = objc_getClass("subclass-3");
   
    id result = objcjs_invoke(aClass, CFRetain(@"An Arg"));
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunctionWith2Args {
    defineJSFunction("subclass-4", impl2);
    Class aClass = objc_getClass("subclass-4");
   
    id result = objcjs_invoke(aClass, CFRetain(@"An Arg"), CFRetain(@"An Arg2"));
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqual(@"An Arg2", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunctionWithVarArgs {
    defineJSFunction("subclass-5", impl3);
    Class aClass = objc_getClass("subclass-5");
   
    id result = objcjs_invoke(aClass, CFRetain(@"An Arg"), CFRetain(@"An Arg2"), CFRetain(@"An Arg3"));
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqualObjects(@([result count]), @3, @"It returns the arg");
}

#pragma mark - Number tests

- (void)testIncrementReturnPlusOne {
    NSNumber *target = [NSNumber numberWithDouble:1.0];
    NSNumber *result = [target objcjs_increment];
    
    XCTAssertEqualObjects(@2.0, result, @"It returns the arg");
    XCTAssertEqualObjects(@2.0, [@1.0 objcjs_increment], @"It returns the arg");
}

- (void)testIncrementDoesntMutateReceiver {
    NSNumber *target = [NSNumber numberWithDouble:1.0];
    [target objcjs_increment];
    XCTAssertEqualObjects(@1.0, target, @"It returns the arg");
}

@end
