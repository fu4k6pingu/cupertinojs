function larryf(argument){
    NSLog("enter");
    
    for (var incrementer = 42; incrementer < 1337; incrementer = incrementer + 1){
        NSLog("%@", incrementer);
    }
    return 0;
}


function objcjs_main(a, b){
    NSLog("zero: %@", larryf(1))
    return 0
}
