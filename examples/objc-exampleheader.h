#import <Foundation/Foundation.h>

@interface Salamander : NSObject

- (void)slide;

@end

@interface NSObject (Extensions)

+ (id)extend:(NSString *)name;

@end