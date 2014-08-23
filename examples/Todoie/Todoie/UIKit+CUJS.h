//
//  UIKit+CUJS.h
//  Todoie
//
//  Created by Jerry Marino on 8/8/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//
//  FIXME : abstract UIKit+CUJS into seperate framework

@interface NSObject (CUJSExtend)

+ (id)extend:(NSString *)name;

@end

@interface UIApplication (CUJS)

// Since CUJS doesn't yet support c function calls,
// wrap this one into a class method
+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName;

@end
