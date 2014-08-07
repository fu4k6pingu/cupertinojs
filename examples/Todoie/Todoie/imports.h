//
//  imports.h
//  Todoie
//
//  Created by Jerry Marino on 8/5/14.
//  Copyright (c) 2014 Jerry Marino. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <UIKit/UIKitDefines.h>
#import <UIKit/UIResponer.h>
#import <UIKit/UIApplication.h>
#import <Foundation/Foundation.h>

@interface NSObject (Extensions)

+ (id)extend:(NSString *)name;
+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName;

@end

@interface UIApplication (ObjCJS)

+ (int)applicationMain:(int)argc argV:(char *[])argv name:(NSString *)appDelegateName;

@end

@interface UIView (ObjCJS)

- (instancetype)initWithFrameValue:(id)frameValue;
- (id)frameValue;

@end
