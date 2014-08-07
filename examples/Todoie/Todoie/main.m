//
//  main.m
//  Todoie
//
//  Created by Jerry Marino on 8/5/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

//#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "objcjs_runtime.h"

@implementation NSObject (ObjCJS)

+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName
{
    @autoreleasepool {
        int value = UIApplicationMain(argc, argv, nil, appDelegateName);
        return value;
    }
}

@end

@interface _TodoieAppDelegate : NSObject  <UIApplicationDelegate>

@end

@implementation _TodoieAppDelegate

-(id)init{
    if (self = [super init]) {
    }
    return self;
}

@end

//int main(int argc, char * argv[])
//{
//    @autoreleasepool {
//        id scope = objcjs_GlobalScope;
//        id instance = [(id)AppDelegate new];
//        NSLog(@"instance %@", instance);
//        [instance application:[UIApplication sharedApplication] didFinishLaunchingWithOptions:@{}];
//        return UIApplicationMain(argc, argv, nil, @"_TodoieAppDelegate");
//    }
//}
