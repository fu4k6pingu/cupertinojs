//
//  UIKit+CUJS.m
//  Todoie
//
//  Created by Jerry Marino on 8/8/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import "UIKit+CUJS.h"
#import <Foundation/Foundation.h>
#import "cujs_runtime.h"
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import <objc/message.h>

@implementation UIApplication (CUJS)

+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName
{
    @autoreleasepool {
        struct MColor color = (struct MColor){1,2,3,4};
        id colorObject = (__bridge id)(objc_Struct(@"MColor", &color));
        NSLog(@"%p", colorObject);
        
        CGRect rect = (CGRect){(double)10.0,10, 6, 6};

        id structObject = (__bridge id)(objc_Struct(@"CGRect", &rect));
        NSLog(@"%@", structObject);

        CGRect retrievedRect;
        NSValue *structValue = [structObject structValue];
        [structValue getValue:&retrievedRect];
        
        NSLog(@"%@", structValue);
        char *bytes = [structObject toStruct];

        NSLog(@"%p", bytes);
        CGRect retrievedRect2;
        memcpy(&retrievedRect2, bytes, sizeof(CGRect));
        free(bytes);
        
        CGPoint pt = (CGPoint){33,44};
        id ptObject = (__bridge id)(objc_Struct(@"CGPoint", &pt));

        CGPoint retrievedPoint;
        NSValue *ptValue = [ptObject structValue];
        [ptValue getValue:&retrievedPoint];
        
        NSString *rectEncode = [[structObject class] performSelector:NSSelectorFromString(@"structEncoding")];
        NSString *ptEncode = [[ptObject class] performSelector:NSSelectorFromString(@"structEncoding")];

        BOOL correctPtEncoding = [ptEncode isEqualToString:[NSString stringWithFormat:@"%s", @encode(CGPoint)]];
        assert(correctPtEncoding);

        BOOL correctRectEncoding = [rectEncode isEqualToString:[NSString stringWithFormat:@"%s", @encode(CGRect)]];
        assert(correctRectEncoding);
       
        assert(!CGFLOAT_IS_DOUBLE);
        UIApplicationMain(argc, argv, nil, @"AppDelegate");
        return 0;
    }
}

@end

@implementation UIView (CUJS)

- (instancetype)initWithFrameValue:(id)frameValue {
    CGRect frame = CGRectMake(
                              [[frameValue cujs_ss_valueForKey:@"x"] floatValue],
                              [[frameValue cujs_ss_valueForKey:@"y"] floatValue],
                              [[frameValue cujs_ss_valueForKey:@"width"] floatValue],
                              [[frameValue cujs_ss_valueForKey:@"height"] floatValue]
                              );
    return [self initWithFrame:frame];
}

- (id)frameValue {
    CGRect frame = self.frame;
    NSObject *frameValue = [NSObject new];
    [frameValue cujs_ss_setValue:@(frame.origin.x) forKey:@"x"];
    [frameValue cujs_ss_setValue:@(frame.origin.y) forKey:@"y"];
    [frameValue cujs_ss_setValue:@(frame.size.width) forKey:@"width"];
    [frameValue cujs_ss_setValue:@(frame.size.height) forKey:@"height"];
    return frameValue;
}

- (struct Animal)animal
{
    NSLog(@"in animal %s", __PRETTY_FUNCTION__);
    struct Animal a;
    a._id = 42;
    return a;
}

@end

