function larryf(argument){
    NSLog("LARRYFFROMJS");
    function jerryf(){
        var a = argument
        return a;
    }
    return jerryf();
}

function objcjs_main(a, b){
    //seg faults when called with an arg
    NSLog("MAINBEFORE %@", 0);
    NSLog("zero: %@", larryf(42));
    NSLog("MAINAFTER %@", 0);
    return 0;
}
