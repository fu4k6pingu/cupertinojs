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

+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName;

@end

@interface UIView (CUJS)

- (instancetype)initWithFrameValue:(id)frameValue;
- (id)frameValue;

@end
