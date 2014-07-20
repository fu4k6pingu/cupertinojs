function larryf(argument){
    NSLog("enter");
    for (var incrementer = 1; incrementer < 10; incrementer = incrementer + 1){
        if (incrementer == argument){
            NSLog("inc eq %@", incrementer);
            return incrementer;
        } else {
            NSLog("inc %@", incrementer);
        }
    }
    return 0;
}

function moe(argument){
    var incrementer = 0;
    do { NSLog("%@", incrementer); incrementer = incrementer + 1; }
    while (incrementer < 3)
}

function objcjs_main(a, b){
    NSLog("zero: %@", larryf(1))
    return 0
}
