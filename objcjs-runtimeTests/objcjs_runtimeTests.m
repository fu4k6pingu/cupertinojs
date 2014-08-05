//
//  objcjs_runtimeTests.m
//  objcjs-runtimeTests
//
//  Created by Jerry Marino on 7/15/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "objcjs_runtime.h"
#import <objc/message.h>

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
    objcjs_defineJSFunction("subclass-1", echoFirstArgImp);
    Class aClass = objc_getClass("subclass-1");
    NSObject *f = [aClass new];
    id result = [f _objcjs_body:@"An Arg"];
    
    XCTAssertTrue(f, @"It creates a subclass");
    XCTAssertTrue(calledBody, @"It creates a subclass");
   
    //TODO : test the impl of body
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

#pragma mark - JSObject

- (void)testItCanCreateAJSObjectClass {
    Class aClass = objcjs_newJSObjectClass();
    XCTAssertNotNil(aClass, @"It creates a class");
}

- (void)testItCanCreateASubclassWithName {
    Class aClass = objcjs_newSubclass([NSObject class], @"Foo");
    XCTAssertTrue(aClass == objc_getClass("Foo"), @"It creates a subclass");
}

- (void)testItCanRespondToNumberProperties {
    Class aClass = objcjs_newJSObjectClass();
    id instance = [aClass new];

    [instance objcjs_defineProperty:"4"];
    SEL expectedSetterSelector = NSSelectorFromString(@"set4:");
    SEL expectedGetterSelector = NSSelectorFromString(@"4");
    XCTAssertTrue([instance respondsToSelector:expectedSetterSelector], @"It adds the setter with the appropriate selector");
    XCTAssertTrue([instance respondsToSelector:expectedGetterSelector], @"It adds the gettter with the appropriate selector");
   
    [instance performSelector:expectedSetterSelector withObject:@"four"];
    id result = objc_msgSend(instance, expectedGetterSelector);

    XCTAssertEqual(@"four", result, @"It returns the value of the property");
}

- (void)testSubscriptingNumbersDefinedByStrings {
    Class aClass = objcjs_newJSObjectClass();
    id instance = [aClass new];
   
    [instance objcjs_replaceObjectAtIndex:@"1" withObject:@"foo"];
    XCTAssertEqual([instance objcjs_objectAtIndex:@"1" ], @"foo", @"It can access properties defined by strings as strings");
    XCTAssertEqual([instance objcjs_objectAtIndex:@1 ], @"foo", @"It can access properties defined by strings as numbers");
}

- (void)testSubscriptingNumbersDefinedByNumbers {
    Class aClass = objcjs_newJSObjectClass();
    id instance = [aClass new];
   
    [instance objcjs_replaceObjectAtIndex:@1 withObject:@"foo"];
    XCTAssertEqual([instance objcjs_objectAtIndex:@"1" ], @"foo", @"It can access properties defined by numbers as strings");
    XCTAssertEqual([instance objcjs_objectAtIndex:@1 ], @"foo", @"It can access properties defined by numbers as numbers");
}

- (void)testItCanDefineAPropety {
    const char *name = "name";
    NSObject *f = [NSObject new];
    [f objcjs_defineProperty:name];
    XCTAssertTrue([f respondsToSelector:NSSelectorFromString(@"name")], @"It adds a getter");
    XCTAssertTrue([f respondsToSelector:NSSelectorFromString(@"setName:")], @"It adds a setter");
    
    [f performSelector:NSSelectorFromString(@"setName:") withObject:@3];
    id nameValue = [f performSelector:NSSelectorFromString(@"name")];
    XCTAssertEqual(nameValue, @3, @"It can set the value");
}

- (void)testItCanSetAParent {
    id parent = [NSObject new];
    objcjs_defineJSFunction("child", echoFirstArgImp);
    Class childClass = objc_getClass("child");
    [childClass _objcjs_setParent:parent];
    XCTAssertEqual([childClass _objcjs_parent], parent, @"It has a parent");
}

- (void)testItCanInvokeAJSFunctionInstance {
    objcjs_defineJSFunction("subclass-2", echoFirstArgImp);
    Class aClass = objc_getClass("subclass-2");
    id f = [aClass new];
    id result = objcjs_invoke(f, CFRetain(@"An Arg"));
    
    XCTAssertTrue(calledBody, @"It invokes body:");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunction {
    objcjs_defineJSFunction("subclass-3", echoFirstArgImp);
    Class aClass = objc_getClass("subclass-3");
   
    id result = objcjs_invoke(aClass, CFRetain(@"An Arg"));
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunctionWith2Args {
    objcjs_defineJSFunction("subclass-4", impl2);
    Class aClass = objc_getClass("subclass-4");
   
    id result = objcjs_invoke(aClass, CFRetain(@"An Arg"), CFRetain(@"An Arg2"));
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqual(@"An Arg2", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunctionWithVarArgs {
    objcjs_defineJSFunction("subclass-5", impl3);
    Class aClass = objc_getClass("subclass-5");
   
    id result = objcjs_invoke(aClass, CFRetain(@"An Arg"), CFRetain(@"An Arg2"), CFRetain(@"An Arg3"));
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqualObjects(@([result count]), @3, @"It returns the arg");
}

- (void)testPropertyAssignmentCreatesANewInstanceMethod {
    objcjs_defineJSFunction("subclass-6", impl3);
    Class aClass = objc_getClass("subclass-6");

    objcjs_defineJSFunction("instance-method-class", echoFirstArgImp);
   
    Class methodClass = objc_getClass("instance-method-class");

    id instance = [aClass new];
    objcjs_assignProperty(instance, "aMethod", methodClass);

    SEL expectedSelector = NSSelectorFromString(@"aMethod:");
    XCTAssertTrue([instance respondsToSelector:expectedSelector], @"It adds the instance method with the selector");
    id result = objc_msgSend(instance, expectedSelector, CFRetain(@"An Arg"), nil);

    XCTAssertEqual(@"An Arg", result, @"It returns the arg of the instance method");
}

- (void)testPropertyAssignmentCreatesANewProperty {
    objcjs_defineJSFunction("subclass-7", impl3);
    Class aClass = objc_getClass("subclass-7");
   
    id instance = [aClass new];
    id value = @1;
    
    objcjs_assignProperty(instance, "value", value);

    SEL expectedSetterSelector = NSSelectorFromString(@"setValue:");
    SEL expectedGetterSelector = NSSelectorFromString(@"value");
    XCTAssertTrue([instance respondsToSelector:expectedSetterSelector], @"It adds the setter with the appropriate selector");
    XCTAssertTrue([instance respondsToSelector:expectedGetterSelector], @"It adds the gettter with the appropriate selector");
    
    id result = objc_msgSend(instance, expectedGetterSelector);

    XCTAssertEqual(@1, result, @"It returns the value of the property");
}


- (void)testPropertyAssignmentCreatesANewClassProperty {
    objcjs_defineJSFunction("subclass-8", impl3);
    Class aClass = objc_getClass("subclass-8");
    id value = @1;
    objcjs_assignProperty(aClass, "value", value);

    SEL expectedSetterSelector = NSSelectorFromString(@"setValue:");
    SEL expectedGetterSelector = NSSelectorFromString(@"value");
    XCTAssertTrue([aClass respondsToSelector:expectedSetterSelector], @"It adds the setter with the appropriate selector");
    XCTAssertTrue([aClass respondsToSelector:expectedGetterSelector], @"It adds the gettter with the appropriate selector");
    
    id result = objc_msgSend(aClass, expectedGetterSelector);

    XCTAssertEqual(@1, result, @"It returns the value of the property");
}

- (void)testPropertyAssignmentCreatesANewClassMethod {
    objcjs_defineJSFunction("subclass-9", impl3);
    Class aClass = objc_getClass("subclass-9");

    objcjs_defineJSFunction("method-class2", echoFirstArgImp);
   
    Class methodClass = objc_getClass("method-class2");

    objcjs_assignProperty(aClass, "aMethod", methodClass);

    SEL expectedSelector = NSSelectorFromString(@"aMethod:");
    XCTAssertTrue([aClass respondsToSelector:expectedSelector], @"It adds the aClass method with the selector");
    id result = objc_msgSend(aClass, expectedSelector, CFRetain(@"An Arg"), nil);

    XCTAssertEqual(@"An Arg", result, @"It returns the arg of the instance method");
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
