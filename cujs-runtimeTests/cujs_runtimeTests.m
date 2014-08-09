//
//  cujs_runtimeTests.m
//  cujs-runtimeTests
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "cujs_runtime.h"
#import <objc/message.h>

@interface cujs_runtimeTests : XCTestCase

@end

@implementation cujs_runtimeTests

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

id echoFirstArgImp(id firstObject,
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
    cujs_defineJSFunction("subclass-1", echoFirstArgImp);
    Class aClass = objc_getClass("subclass-1");
    NSObject *f = [aClass new];
    id result = [f _cujs_body:@"An Arg"];
    
    XCTAssertTrue(f, @"It creates a subclass");
    XCTAssertTrue(calledBody, @"It creates a subclass");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

#pragma mark - JSObject

- (void)testItCanCreateAJSObjectClass {
    Class aClass = cujs_newJSObjectClass();
    XCTAssertNotNil(aClass, @"It creates a class");
}

- (void)testItCanCreateASubclassWithName {
    Class aClass = cujs_newSubclass([NSObject class], @"Foo");
    XCTAssertTrue(aClass == objc_getClass("Foo"), @"It creates a subclass");
}

- (void)testItCanSubclassMethods {
    Class MyError = cujs_newSubclass([NSError class], @"MyError");

    Class bodyClass = cujs_defineJSFunction("MyErrorBody", echoFirstArgImp);

    cujs_assignProperty(MyError, "errorWithDomain:code:userInfo:", bodyClass);

    id result = [(id)MyError errorWithDomain:@"four" code:3 userInfo:@{}];
    
    XCTAssertEqual(@"four", result, @"It returns the value of the property");
    
}

- (void)testItCanRespondToNumberProperties {
    Class aClass = cujs_newJSObjectClass();
    id instance = [aClass new];

    [instance cujs_defineProperty:"4"];
    SEL expectedSetterSelector = NSSelectorFromString(@"set4:");
    SEL expectedGetterSelector = NSSelectorFromString(@"4");
    XCTAssertTrue([instance respondsToSelector:expectedSetterSelector], @"It adds the setter with the appropriate selector");
    XCTAssertTrue([instance respondsToSelector:expectedGetterSelector], @"It adds the gettter with the appropriate selector");
   
    [instance performSelector:expectedSetterSelector withObject:@"four"];
    id result = objc_msgSend(instance, expectedGetterSelector);

    XCTAssertEqual(@"four", result, @"It returns the value of the property");
}

- (void)testSubscriptingNumbersDefinedByStrings {
    Class aClass = cujs_newJSObjectClass();
    id instance = [aClass new];
   
    [instance cujs_ss_setValue:@"foo" forKey:@"1" ];
    XCTAssertEqual([instance cujs_ss_valueForKey:@"1" ], @"foo", @"It can access properties defined by strings as strings");
    XCTAssertEqual([instance cujs_ss_valueForKey:@1 ], @"foo", @"It can access properties defined by strings as numbers");
}

- (void)testSubscriptingNumbersDefinedByNumbers {
    Class aClass = cujs_newJSObjectClass();
    id instance = [aClass new];
   
    [instance cujs_ss_setValue:@"foo" forKey:@1];
    XCTAssertEqual([instance cujs_ss_valueForKey:@"1" ], @"foo", @"It can access properties defined by numbers as strings");
    XCTAssertEqual([instance cujs_ss_valueForKey:@1 ], @"foo", @"It can access properties defined by numbers as numbers");
}

- (void)testItCanDefineAPropety {
    const char *name = "name";
    NSObject *f = [NSObject new];
    [f cujs_defineProperty:name];
    XCTAssertTrue([f respondsToSelector:NSSelectorFromString(@"name")], @"It adds a getter");
    XCTAssertTrue([f respondsToSelector:NSSelectorFromString(@"setName:")], @"It adds a setter");
    
    [f performSelector:NSSelectorFromString(@"setName:") withObject:@3];
    id nameValue = [f performSelector:NSSelectorFromString(@"name")];
    XCTAssertEqual(nameValue, @3, @"It can set the value");
}

- (void)testItCanSetAParent {
    id parent = [NSObject new];
    cujs_defineJSFunction("child", echoFirstArgImp);
    Class childClass = objc_getClass("child");
    [childClass _cujs_setParent:parent];
    XCTAssertEqual([childClass _cujs_parent], parent, @"It has a parent");
}

- (void)testItCanInvokeAJSFunctionInstance {
    cujs_defineJSFunction("subclass-2", echoFirstArgImp);
    Class aClass = objc_getClass("subclass-2");
    id f = [aClass new];
    id result = cujs_invoke(f, CFRetain(@"An Arg"));
    
    XCTAssertTrue(calledBody, @"It invokes body:");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunction {
    cujs_defineJSFunction("subclass-3", echoFirstArgImp);
    Class aClass = objc_getClass("subclass-3");
   
    id result = cujs_invoke(aClass, CFRetain(@"An Arg"));
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunctionWith2Args {
    cujs_defineJSFunction("subclass-4", impl2);
    Class aClass = objc_getClass("subclass-4");
   
    id result = cujs_invoke(aClass, CFRetain(@"An Arg"), CFRetain(@"An Arg2"));
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqual(@"An Arg2", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunctionWithVarArgs {
    cujs_defineJSFunction("subclass-5", impl3);
    Class aClass = objc_getClass("subclass-5");
   
    id result = cujs_invoke(aClass, CFRetain(@"An Arg"), CFRetain(@"An Arg2"), CFRetain(@"An Arg3"));
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqualObjects(@([result count]), @3, @"It returns the arg");
}

- (void)testPropertyAssignmentCreatesANewInstanceMethod {
    cujs_defineJSFunction("subclass-6", impl3);
    Class aClass = objc_getClass("subclass-6");

    cujs_defineJSFunction("instance-method-class", echoFirstArgImp);
   
    Class methodClass = objc_getClass("instance-method-class");

    id instance = [aClass new];
    cujs_assignProperty(instance, "aMethod", methodClass);

    SEL expectedSelector = NSSelectorFromString(@"aMethod");
    XCTAssertTrue([instance respondsToSelector:expectedSelector], @"It adds the instance method with the selector");
    id result = objc_msgSend(instance, expectedSelector, CFRetain(@"An Arg"), nil);

    XCTAssertEqual(@"An Arg", result, @"It returns the arg of the instance method");
}

- (void)testPropertyAssignmentCreatesANewProperty {
    cujs_defineJSFunction("subclass-7", impl3);
    Class aClass = objc_getClass("subclass-7");
   
    id instance = [aClass new];
    id value = @1;
    
    cujs_assignProperty(instance, "value", value);

    SEL expectedSetterSelector = NSSelectorFromString(@"setValue:");
    SEL expectedGetterSelector = NSSelectorFromString(@"value");
    XCTAssertTrue([instance respondsToSelector:expectedSetterSelector], @"It adds the setter with the appropriate selector");
    XCTAssertTrue([instance respondsToSelector:expectedGetterSelector], @"It adds the gettter with the appropriate selector");
    
    id result = objc_msgSend(instance, expectedGetterSelector);

    XCTAssertEqual(@1, result, @"It returns the value of the property");
}


- (void)testPropertyAssignmentCreatesANewClassProperty {
    cujs_defineJSFunction("subclass-8", impl3);
    Class aClass = objc_getClass("subclass-8");
    id value = @1;
    cujs_assignProperty(aClass, "value", value);

    SEL expectedSetterSelector = NSSelectorFromString(@"setValue:");
    SEL expectedGetterSelector = NSSelectorFromString(@"value");
    XCTAssertTrue([aClass respondsToSelector:expectedSetterSelector], @"It adds the setter with the appropriate selector");
    XCTAssertTrue([aClass respondsToSelector:expectedGetterSelector], @"It adds the gettter with the appropriate selector");
    
    id result = objc_msgSend(aClass, expectedGetterSelector);

    XCTAssertEqual(@1, result, @"It returns the value of the property");
}

- (void)testPropertyAssignmentCreatesANewClassMethod {
    cujs_defineJSFunction("subclass-9", impl3);
    Class aClass = objc_getClass("subclass-9");

    cujs_defineJSFunction("method-class2", echoFirstArgImp);
   
    Class methodClass = objc_getClass("method-class2");

    cujs_assignProperty(aClass, "aMethod", methodClass);

    SEL expectedSelector = NSSelectorFromString(@"aMethod");
    XCTAssertTrue([aClass respondsToSelector:expectedSelector], @"It adds the aClass method with the selector");
    id result = objc_msgSend(aClass, expectedSelector, CFRetain(@"An Arg"), nil);

    XCTAssertEqual(@"An Arg", result, @"It returns the arg of the instance method");
}

#pragma mark - Number tests

- (void)testIncrementReturnPlusOne {
    NSNumber *target = [NSNumber numberWithDouble:1.0];
    NSNumber *result = [target cujs_increment];
    
    XCTAssertEqualObjects(@2.0, result, @"It returns the arg");
    XCTAssertEqualObjects(@2.0, [@1.0 cujs_increment], @"It returns the arg");
}

- (void)testIncrementDoesntMutateReceiver {
    NSNumber *target = [NSNumber numberWithDouble:1.0];
    [target cujs_increment];
    XCTAssertEqualObjects(@1.0, target, @"It returns the arg");
}

@end
