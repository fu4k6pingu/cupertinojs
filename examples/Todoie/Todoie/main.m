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

int main(int argc, char * argv[])
{
    @autoreleasepool {
        id scope = objcjs_GlobalScope;
//        [NSObject objcjs_new];
        NSLog(@"%@ scope", scope);
        return UIApplicationMain(argc, argv, nil, @"UIResponder");
    }
}
