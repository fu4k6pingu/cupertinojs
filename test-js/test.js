//objc_import("examples/objc-exampleheader.h")

function Counter(init){
    this.value = init
    this.increment = function(){
        this.value = this.value + 1
        return this.value
    }

    return this
}

function main(a,b){
    var makeRun  = function(){
        var c = new Counter(9)
        NSLog("incremented %@", c.increment())
        return this
    }
    
    makeRun()
    
    var zero = 0
    return zero.intValue
}