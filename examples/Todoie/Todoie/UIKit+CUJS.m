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
        //FIXME : this stuff really belongs in some type of integration
        // test and not the example - ignore this for now
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
