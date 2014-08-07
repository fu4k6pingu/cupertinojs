#import <Foundation/Foundation.h>
//#import <UIKit/UIKit.h>

@interface NSObject (ObjCJS)

+ (id)extend:(id)arg;

@end

struct SalamanderState {int color:0};

@interface Salamander : NSObject

- (void)slide;
- (struct SalamanderState)salamanderState;

@end
