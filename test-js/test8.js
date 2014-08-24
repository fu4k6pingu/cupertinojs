function larryf(argument){
    var a = argument
    
    function nestedFunction(){
        function nestedFunction2(){
            a = a + 1
            return a + 1;
        }
        
        return nestedFunction2();
    }
    return nestedFunction
}

function main(a, b){
    var counter = larryf(0);
    NSLog("c: %@", counter);
    
    NSLog("zero: %@", counter())
    NSLog("one: %@", counter())
    NSLog("two: %@", counter())
    NSLog("three: %@", counter())
    return 0
}
