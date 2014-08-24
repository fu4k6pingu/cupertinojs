typedef float CGFloat;

struct CGPoint {
    CGFloat x;
    CGFloat y;
};
typedef struct CGPoint CGPoint;

/* Sizes. */

struct CGSize {
    CGFloat width;
    CGFloat height;
};
typedef struct CGSize CGSize;

struct CGRect {
    CGPoint origin;
    CGSize size;
};
typedef struct CGRect CGRect;

@interface NSObject (CUJS)

+ (id)extend:(id)arg;

@end

struct Color { int r; int g; int b; int a; };
struct Animal { struct Color color; int _id; };
struct MColor { int r; int g; int b; int a; };

@interface ZooObject : NSObject

- (void)slide;
- (struct Animal)animal;

@end

@interface console : NSObject

+ (void)log:(id)arg, ...;

@end
