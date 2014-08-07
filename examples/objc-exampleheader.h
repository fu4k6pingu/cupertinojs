#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface Salamander : NSObject

- (void)slide;

@end

@interface NSObject (Extensions)

+ (id)extend:(NSString *)name;

@end