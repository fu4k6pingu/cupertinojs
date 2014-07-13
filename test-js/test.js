function test(condition){
    if (condition){
        return 1;
    } else {
        return 0;
    }
}

function objcjs_main(a, b){
    NSLog("one: %f \n", test(1));
    NSLog("two: %f \n", test(2));
    NSLog("zero: %f \n", test(0));
    return 0;
}
