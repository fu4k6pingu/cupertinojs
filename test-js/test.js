function larryf(argument){
    var a = 3
    function nestedFName(){
        var c = a;
        return c;
    }
   
    return nestedFName();
}

function objcjs_main(a, b){
    //seg faults when called with an arg
    NSLog("zero: %@", larryf(4));
    return 0;
}
