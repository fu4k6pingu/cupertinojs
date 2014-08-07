objc_import("examples/objc-exampleheader.h")

function Counter(init){
    this.value = init
    
    function increment(){
        this.value = this.value + 1
    }
    
    this.increment = increment
    return this
}

function ObjCInt(value){
    return value.intValue
}

function makeRun(){
    function log(){
        NSLog("Called")
        return this
    }
    
    NSError.prototype.log = log
    var error = NSError.alloc.init;

    error.log(9)
}

function main(a, b){
    makeRun()

    
    return ObjCInt(0)
}
