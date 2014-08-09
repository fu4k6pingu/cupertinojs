objc_import("test-imports.h")

function DidLoad(){
//    var color = Color.alloc.init
    var color = objc_Struct("MColor", null);
    NSLog("Color class %@", color.class);

    NSLog("Color r %@", color.r);
    NSLog("Color g %@", color.g);
    NSLog("Color b %@", color.b);
    NSLog("Color a %@", color.a);
}

function main(a,b){
    DidLoad()
    var zero = 0
    return zero.intValue
}