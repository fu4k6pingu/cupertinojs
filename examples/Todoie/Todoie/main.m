//
//  main.m
//  Todoie
//
//  Created by Jerry Marino on 8/5/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import <UIKit/UIKit.h>
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
                              [[frameValue objcjs_objectAtIndex:@"x"] floatValue],
                              [[frameValue objcjs_objectAtIndex:@"y"] floatValue],
                              [[frameValue objcjs_objectAtIndex:@"width"] floatValue],
                              [[frameValue objcjs_objectAtIndex:@"height"] floatValue]
                              );
    return [self initWithFrame:frame];
}

- (id)frameValue {
    CGRect frame = self.frame;
    NSObject *frameValue = [NSObject new];
    [frameValue objcjs_replaceObjectAtIndex:@"x"
                                 withObject:@(frame.origin.x)];
    [frameValue objcjs_replaceObjectAtIndex:@"y"
                                 withObject:@(frame.origin.y)];
    [frameValue objcjs_replaceObjectAtIndex:@"width"
                                 withObject:@(frame.size.width)];
    [frameValue objcjs_replaceObjectAtIndex:@"height"
                                 withObject:@(frame.size.height)];
    return frameValue;
}

@end
