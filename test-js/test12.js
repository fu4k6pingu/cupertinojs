function Counter(init){
    this.value = init
    
    function increment(){
        this.value = this.value + 1
    }
    
    this.increment = increment
    return this
}

function objcjs_main(a, b){
    var c = new Counter(1)
    c.increment()
    c.increment()
   
    NSLog("retainCt %d", c.retainCount)
    NSLog("final %@", c.value)
    
    return 0
}

