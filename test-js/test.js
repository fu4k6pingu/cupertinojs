function larryf(argument){
    var a = argument
   
    function nestedFName(){
        return a
    }
   
    return nestedFName()
}

function objcjs_main(a, b){
    var b = larryf(4)
    NSLog("zero: %@", b)
    return 0
}
