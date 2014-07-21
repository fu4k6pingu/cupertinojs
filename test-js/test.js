function larryf(argument, argument2){
    NSLog("enter");
    for (var incrementer = 1; incrementer < 10; incrementer = incrementer + 1){
        if (incrementer == argument2){
            NSLog("inc eq %@", incrementer);
            return incrementer;
        } else {
            NSLog("inc %@", incrementer);
        }
    }
    return 0;
}

function moe(){
    var incrementer = 0;
    do { NSLog("%@", incrementer); incrementer = incrementer + 1; }
    while (incrementer < 3)
}

function objcjs_main(a, b){
    NSLog("larry: %@", larryf(2, 3))
    NSLog("moe: %@", moe(3, 4, 5))
    return 0
}
