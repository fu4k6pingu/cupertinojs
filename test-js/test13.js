function Counter(init){
    this.value = init
    
    function increment(){
        this.value = this.value + 1
    }
    
    this.increment = increment
    return this
}

function foo(){
    var c = new Counter(1)
    c.increment()
    c.increment()
    NSLog("final %@", c.value)
}

function cujs_main(a, b){
    foo()
    //c will be deallocated
    foo()
    return 0
}

