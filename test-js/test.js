function larryf(argument){
    var a = argument
    if (a){
        return 10;
    } else {
        return 3
    }
}

function counter(argument){
    var a = argument;
    function barry(){
        return a + 1
    }
    return barry;
}

function objcjs_main(a, b){
    NSLog("zero: %@", larryf(1))
    NSLog("one: %@", larryf(2))
    NSLog("two: %@", larryf(2))
    NSLog("three: %@", larryf2(3))
    
    var c = counter(0)
    NSLog("zero: %@", c(1))
    NSLog("one: %@", c(2))
    NSLog("two: %@", c(2))
    NSLog("three: %@", c(3))
    return 0
}
