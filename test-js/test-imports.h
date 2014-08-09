#import <Foundation/Foundation.h>

@interface NSObject (CUJS)

+ (id)extend:(id)arg;

@end

struct SalamanderState { int color:0 };

@interface Salamander : NSObject

- (void)slide;
- (struct SalamanderState)salamanderState;

@end
