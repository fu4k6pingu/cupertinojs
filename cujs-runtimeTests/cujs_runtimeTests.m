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
    id result = cujs_invoke(f, CFRetain(@"An Arg"), nil);
    
    XCTAssertTrue(calledBody, @"It invokes body:");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunction {
    cujs_defineJSFunction("subclass-3", echoFirstArgImp);
    Class aClass = objc_getClass("subclass-3");
   
    id result = cujs_invoke(aClass, CFRetain(@"An Arg"), nil);
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqual(@"An Arg", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunctionWith2Args {
    cujs_defineJSFunction("subclass-4", impl2);
    Class aClass = objc_getClass("subclass-4");
   
    id result = cujs_invoke(aClass, CFRetain(@"An Arg"), CFRetain(@"An Arg2"), nil);
    XCTAssertTrue(calledBody, @"It invokes body: on a new instance");
    XCTAssertEqual(@"An Arg2", result, @"It returns the arg");
}

- (void)testItCanInvokeAJSFunctionWithVarArgs {
    cujs_defineJSFunction("subclass-5", impl3);
    Class aClass = objc_getClass("subclass-5");
   
    id result = cujs_invoke(aClass, CFRetain(@"An Arg"), CFRetain(@"An Arg2"), CFRetain(@"An Arg3"), nil);
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


@interface cujs_runtimeStructTests : XCTestCase

@end

struct SalamanderState { int hot; int spots; };

@interface Salamander : NSObject

@property (nonatomic) struct SalamanderState state;

@end

@implementation Salamander

@synthesize state;

@end

@implementation cujs_runtimeStructTests

#pragma mark - Struct tests

- (void)setUp {
    [super setUp];

}

- (void)testSanityStandardStructAssignment {
    Salamander *salamander = [Salamander new];
    struct SalamanderState state;
    state.spots = 42;
    state.hot = 1;
    salamander.state = state;
    
    XCTAssertTrue(salamander.state.hot == 1, @"It is hot");
    XCTAssertTrue(salamander.state.spots == 42, @"It has a spots");
}

- (void)testSanityStructAssignemntWithNSValue {
    int offsetOfHot = offsetof(struct SalamanderState, hot);
    int offsetOfColor = offsetof(struct SalamanderState, spots);
    int sizeOfSalamanderState = sizeof(struct SalamanderState);
    char *encoding = @encode(struct SalamanderState);
    assert(offsetOfHot == 0);
    assert(offsetOfColor == 4);
    assert(sizeOfSalamanderState == 8);
    assert(encoding == "{SalamanderState=ii}");
    
    int *structBytes = malloc(sizeof(struct SalamanderState ));
    char *baseAddr = (char *)structBytes;
    *(int *)(baseAddr + 0) = 1;
    *(int *)(baseAddr + 4) = 42;

    NSValue *value = [NSValue valueWithBytes:structBytes
                                    objCType:@encode(struct SalamanderState)];
    
    Salamander *salamander = [Salamander new];
    [salamander setValue:value forKey:@"state"];
    
    XCTAssertTrue(salamander.state.hot == 1, @"It is hot");
    XCTAssertTrue(salamander.state.spots == 42, @"It has 42 spots");
    
    char structValue[8];
    [value getValue:&structValue];
    XCTAssertTrue(structValue[0] == 1, @"It is hot");
    XCTAssertTrue(structValue[4] == 42, @"It has a spots");
}

- (void)testItCanCreateStructs {
    Class structClass = cujs_newJSObjectClass();
    cujs_registerStruct("{SalamanderState=ii}");
    cujs_assignProperty(structClass, "structType", @"{SalamanderState=ii}");
    cujs_assignProperty(structClass,
                        "fieldOffsets",
                        [@{
                          @"hot" : @0,
                          @"spots": @4
                          } copy]);
    
    cujs_assignProperty(structClass,
                        "structFieldEncoding",
                        [@{
                          @"hot" : @"i",
                          @"spots": @"i"
                          } copy]);
    
    cujs_assignProperty(structClass, "structSize", @8);
    cujs_assignProperty(structClass, "structEncoding", @"{SalamanderState=ii}");
    
    id instance = [[[structClass alloc] init] retain];
    cujs_assignProperty(instance, "hot", @1);
    cujs_assignProperty(instance, "spots", @42);
    
    Salamander *salamander = [[Salamander alloc] init];
    assert(salamander);
    NSValue *structValue = [instance structValue];
    cujs_assignProperty(salamander, "state", structValue);

    XCTAssertTrue(salamander.state.hot == 1, @"It is hot");
    XCTAssertTrue(salamander.state.spots == 42, @"It has 42 spots");
}

struct ZooKeeper {int size; int gender; };

__attribute__((constructor))void initZooZeeper(){
    CUJSStructField fields[] = {
        {"size", "", 0, ObjCType_Int},
        {"gender","", sizeof(int), ObjCType_Int}
    };
    
    cujs_defineStruct("ZooKeeper", 2, fields);
}

- (void)testItCanCreateStructsWithRuntimeFunction {
    // this is really an implementation detail that the
    // structs are prefixed but makes for a good integration test
    id structClass = objc_getClass("JSStructZooKeeper");

    id zookeyperObject = [[structClass alloc] init];
    cujs_assignProperty(zookeyperObject, "size", @69);
    cujs_assignProperty(zookeyperObject, "gender", @3);

    struct ZooKeeper *zookeyper = (struct ZooKeeper *)[zookeyperObject toStruct];
    
    XCTAssertTrue(zookeyper->size == 69, @"It has a size");
    XCTAssertTrue(zookeyper->gender == 3, @"It has a gender");
}

- (void)testItCanCreateStructsWithAStructArgument
{
    id zookeyperObject = objc_Struct(@"ZooKeeper", &(struct ZooKeeper){69, 3});
    XCTAssertTrue([[zookeyperObject performSelector:NSSelectorFromString(@"size")] intValue] == 69, @"It has a size");
    XCTAssertTrue([[zookeyperObject performSelector:NSSelectorFromString(@"gender")] intValue] == 3, @"It has a gender");
}

struct ZooKeeper2 {float size; int gender; };

__attribute__((constructor))void initZooZeeper2(){
    CUJSStructField fields[] = {
        {"size", "", 0, ObjCType_Float},
        {"gender", "", sizeof(float), ObjCType_Int}
    };
    
    cujs_defineStruct("ZooKeeper2", 2, fields);
}

- (void)testItCanCreateStructsWithAStructArgumentAndFloats
{
    id zookeyperObject = objc_Struct(@"ZooKeeper2", &(struct ZooKeeper2){69.3, 3});
    //FIXME : this test is a copout but xcttest can't assert float equality
    XCTAssertEqual((int)[[zookeyperObject performSelector:NSSelectorFromString(@"size")] floatValue], (int)69.3, @"It has a size");
    XCTAssertTrue([[zookeyperObject performSelector:NSSelectorFromString(@"gender")] intValue] == 3, @"It has a gender");
}

struct Color { int r; int g; int b; int a; };
struct Animal { struct Color color; int _id; };

__attribute__((constructor))void initAnimal(){

}

- (void)testItCanCreateStructObjectsWithComplexTypes {
    CUJSStructField colorFields[] = {
        {"r", "", 0, ObjCType_Int},
        {"g", "", sizeof(int), ObjCType_Int},
        {"b", "", sizeof(int) * 2, ObjCType_Int},
        {"a", "", sizeof(int) * 3, ObjCType_Int},
    };
    cujs_defineStruct("Color", 4, colorFields);
  
    CUJSStructField animalFields[] = {
        {"color", "Color", 0, ObjCType_Unexposed},
        {"_id", "", sizeof(struct Color), ObjCType_Int}
    };
    
    cujs_defineStruct("Animal", 2, animalFields);
    
    struct Color color = (struct Color){1, 2, 3, 0};
    struct Animal animal = (struct Animal){color, 42};
    id zookeyperObject = objc_Struct(@"Animal", &animal);
   
    XCTAssertEqualObjects([zookeyperObject performSelector:NSSelectorFromString(@"_id")], @42, @"It has an id");
    
    id colorObject = [zookeyperObject performSelector:NSSelectorFromString(@"color")];
    XCTAssertNotNil(colorObject, @"It has a color");

    XCTAssertEqualObjects([colorObject performSelector:NSSelectorFromString(@"r")], @1, @"It has an r");
    XCTAssertEqualObjects([colorObject performSelector:NSSelectorFromString(@"g")], @2, @"It has an g");
    XCTAssertEqualObjects([colorObject performSelector:NSSelectorFromString(@"b")], @3, @"It has an b");
    XCTAssertEqualObjects([colorObject performSelector:NSSelectorFromString(@"a")], @0, @"It has an a");
}

- (void)testItCanCreateStructsWithComplexTypesToNSValue {
    struct Color color = (struct Color){1, 2, 3, 0};
    struct Animal animal = (struct Animal){color, 42};
    id zookeyperObject = objc_Struct(@"Animal", &animal);

    NSValue *structValue = [zookeyperObject structValue];
    assert(structValue);
    
    struct Animal retrievedAnimal;
    [structValue getValue:&retrievedAnimal];

    XCTAssertEqual(retrievedAnimal._id, 42, @"It has an _id");
    
    struct Color retrievedColor = retrievedAnimal.color;
    XCTAssertEqual(retrievedColor.r, 1, @"It has an r");
    XCTAssertEqual(retrievedColor.g, 2, @"It has an g");
    XCTAssertEqual(retrievedColor.b, 3, @"It has an b");
    XCTAssertEqual(retrievedColor.a, 0, @"It has an a");
}

- (void)testItCanCreateStructsWithToStruct {
    struct Color color = (struct Color){1, 2, 3, 0};
    struct Animal animal = (struct Animal){color, 42};
    id zookeyperObject = objc_Struct(@"Animal", &animal);
    
    struct Animal retrievedAnimal;
    char *bytes  = [zookeyperObject toStruct];
    memcpy(&retrievedAnimal, bytes, sizeof(struct Animal));
    free(bytes);
    
    XCTAssertEqual(retrievedAnimal._id, 42, @"It has an _id");
    
    struct Color retrievedColor = retrievedAnimal.color;
    XCTAssertEqual(retrievedColor.r, 1, @"It has an r");
    XCTAssertEqual(retrievedColor.g, 2, @"It has an g");
    XCTAssertEqual(retrievedColor.b, 3, @"It has an b");
}

@end

