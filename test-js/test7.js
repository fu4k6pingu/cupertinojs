function larryf(argument){
    NSLog("LARRYFFROMJS");
    function jerryf(){
        return 4;
    }
    return jerryf();
}


function main(a, b){
    //seg faults when called with an arg
    NSLog("MAINBEFORE %@", 0);
    NSLog("zero: %@", larryf(42));
    NSLog("MAINAFTER %@", 0);
    return 0;
}
