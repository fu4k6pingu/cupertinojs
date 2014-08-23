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
        UIApplicationMain(argc, argv, nil, name);
        return 0;
    }
}

@end
