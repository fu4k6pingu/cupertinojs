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

- (void)setUp
{
    [super setUp];
}

- (void)tearDown
{
    [super tearDown];
}

static BOOL calledBody = NO;

id impl1(id firstObject, ...){
    calledBody = YES;
    
    va_list argumentList;
    va_start(argumentList, firstObject);
    id instance = firstObject;
    NSLog(@"%@", instance);
    
    SEL selector = va_arg(argumentList, SEL);
    NSLog(@"%s", sel_getName(selector));
    
    id arg3 = va_arg(argumentList, id);
    NSLog(@"%@", arg3);
    va_end(argumentList);
    return arg3;
}

id impl2(id firstObject, void *second, void *third, ...){
    calledBody = YES;
    
    va_list argumentList;
    va_start(argumentList, firstObject);
    id instance = firstObject;
    NSLog(@"%@", instance);
    
    SEL selector = va_arg(argumentList, SEL);
    NSLog(@"%s", sel_getName(selector));
    
    id arg3 = va_arg(argumentList, id);
    NSLog(@"%@", arg3);
    va_end(argumentList);
    return arg3;
}

- (void)testItCanCreateASubclass
{
    Class aClass = (__bridge Class)(defineJSFunction("subclass-1", impl1));
    JSFunction *f = [aClass new];
    id result = [f body:@"An Arg"];
    
    XCTAssertTrue(f, @"It creates a subclass");
    XCTAssertTrue(calledBody, @"It creates a subclass");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

@end
