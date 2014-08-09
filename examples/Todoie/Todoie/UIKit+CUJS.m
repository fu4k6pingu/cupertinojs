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

@implementation UIApplication (CUJS)

+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName
{
    @autoreleasepool {
        UIApplicationMain(argc, argv, nil, appDelegateName);
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

@end

