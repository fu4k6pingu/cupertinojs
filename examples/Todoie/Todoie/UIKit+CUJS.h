//
//  UIKit+CUJS.h
//  Todoie
//
//  Created by Jerry Marino on 8/8/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//
//  FIXME : abstract UIKit+CUJS into seperate framework

struct MColor { int r; int g; int b; int a; };
struct Animal { struct MColor color; int _id; };

@interface NSObject (CUJSExtend)

+ (id)extend:(NSString *)name;

@end

@interface UIApplication (CUJS)

+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName;

@end

@interface UIView (CUJS)

- (instancetype)initWithFrameValue:(id)frameValue;
- (id)frameValue;
- (struct Animal)animal;

@end
