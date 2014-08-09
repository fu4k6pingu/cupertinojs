//
//  UIKit+ObjCJS.h
//  Todoie
//
//  Created by Jerry Marino on 8/8/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//
//  FIXME : abstract UIKit+ObjCJS into seperate framework

@interface UIApplication (ObjCJS)

+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName;

@end

@interface UIView (ObjCJS)

- (instancetype)initWithFrameValue:(id)frameValue;
- (id)frameValue;

@end
