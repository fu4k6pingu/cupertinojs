function test(condition){
    if (condition){
        return 1337;
    } else {
        return 44;
    }
    return 666;
}

function objcjs_main(a, b){
    NSLog("one: %f \n", test(1));
    NSLog("zero: %f \n", test(0));
    return 0;
}
