function larryf(argument){
    NSLog("enter");
    
    for (var incrementer = 1.0; incrementer < 2.0; incrementer = incrementer + 1){
        NSLog("%@", incrementer);
    }
    return 0;
}


function objcjs_main(a, b){
    NSLog("zero: %@", larryf(1))
    return 0
}
