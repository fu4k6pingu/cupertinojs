//
//  UIKit+ObjCJS.m
//  Todoie
//
//  Created by Jerry Marino on 8/8/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import "UIKit+ObjCJS.h"
#import <Foundation/Foundation.h>
#import "objcjs_runtime.h"

@implementation UIApplication (ObjCJS)

+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName
{
    @autoreleasepool {
        UIApplicationMain(argc, argv, nil, appDelegateName);
        return 0;
    }
}

@end

@implementation UIView (ObjCJS)

- (instancetype)initWithFrameValue:(id)frameValue {
    CGRect frame = CGRectMake(
                              [[frameValue objcjs_ss_valueForKey:@"x"] floatValue],
                              [[frameValue objcjs_ss_valueForKey:@"y"] floatValue],
                              [[frameValue objcjs_ss_valueForKey:@"width"] floatValue],
                              [[frameValue objcjs_ss_valueForKey:@"height"] floatValue]
                              );
    return [self initWithFrame:frame];
}

- (id)frameValue {
    CGRect frame = self.frame;
    NSObject *frameValue = [NSObject new];
    [frameValue objcjs_ss_setValue:@(frame.origin.x) forKey:@"x"];
    [frameValue objcjs_ss_setValue:@(frame.origin.y) forKey:@"y"];
    [frameValue objcjs_ss_setValue:@(frame.size.width) forKey:@"width"];
    [frameValue objcjs_ss_setValue:@(frame.size.height) forKey:@"height"];
    return frameValue;
}

@end
