function test(condition){
    if (condition) {
        return 2;
    } else {
        return 0;
    }
}

function objcjs_main(a, b){
    NSLog("one: %@ \n", test(1));
    NSLog("zero: %@ \n", test(0));
    NSLog("null %@ \n", test(null));
    return 0;
}
