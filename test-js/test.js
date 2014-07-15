function test(condition){
    if (condition){
        return 10;
    } else {
        if (1)
            return 4;
    }
    return 666;
}

function objcjs_main(a, b){
    NSLog("one: %@ \n", test(1));
    NSLog("zero: %@ \n", test(0));
    NSLog("null %@ \n", test(null));
    return 1;
}
