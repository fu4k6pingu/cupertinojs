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
function makeError(){
    //NSError is exposed via the implicit global
    //variable 'NSError' because of our import
    NSLog("Error class %@", NSError)
    
    //Extend NSError
//    var JSCountedError = NSError["extend:"]("JSCountedError")
    var JSCountedError = NSError.extend("JSCountedError")
    var counter = new Counter(0)
    
    function CountedErrorFactory(domain, code, userInfo){
        counter.increment(0)
        NSLog("CountedErrorFactory errors created %@", counter.value)
        
        return NSError.errorWithDomainCodeUserInfo(domain,
                                                   code,
                                                   userInfo)
    }
    
    JSCountedError.errorWithDomainCodeUserInfo = CountedErrorFactory
    
    NSLog("CountedErrorFactory - parent %@", CountedErrorFactory._objcjs_parent);
    
    var fancyError = JSCountedError.errorWithDomainCodeUserInfo("Too much whisky!",
                                                                ObjCInt(1984),
                                                                null)
    NSLog("fancyError %@", fancyError)
    
    return fancyError
}

function makeRun(){
    makeError()
    function description(a, b){
        return a
    }
    
    NSError.prototype.description = description
    
    var error = NSError.alloc.init;
    NSLog("description %@", error.description("Argumento"))
}

function main(a, b){
    makeRun()

    return ObjCInt(0)
}
