function larryf(argument){
    NSLog("enter");
    for (var incrementer = 1.0; incrementer < 20.0; incrementer = incrementer + 1){
        if (incrementer == 5.0)
            return 10;
        else
            NSLog("go");
        NSLog("%@", incrementer);
    }
    return 0;
}


function objcjs_main(a, b){
    NSLog("zero: %@", larryf(1))
    return 0
}
