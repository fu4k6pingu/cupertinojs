function larryf(argument){
    NSLog("LARRYFFROMJS");
    return argument;
}


function objcjs_main(a, b){
    //seg faults when called with an arg
    NSLog("MAINBEFORE %@", 0);
    NSLog("zero: %@", larryf(1));
    NSLog("MAINAFTER %@", 0);
    return 0;
}
