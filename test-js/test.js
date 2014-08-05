// Fixme : this needs to be here!!
// it is an unused function but forces the import macros to be parsed first

function ObjCImport(){
    objc_import("examples/objc-exampleheader.h")
}

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
    var JSCountedError = NSError.extend("JSCountedError")
    
    function CountedErrorFactory(domain, code, userInfo){
        this.counter.increment(0)
        NSLog("CountedErrorFactory errors created %@", this.counter.value)
        
        return NSError.errorWithDomainCodeUserInfo(domain,
                                                   code,
                                                   userInfo)
    }
   
    //Override like a boss
    //TODO : this should allow setting of methods with the :
    JSCountedError["errorWithDomain:code:userInfo"] = CountedErrorFactory
    JSCountedError.counter = new Counter(0)
    
    var fancyError = JSCountedError.errorWithDomainCodeUserInfo("Too much whisky!",
                                                                ObjCInt(1984),
                                                                null)
    NSLog("fancyError %@", fancyError)
    
    return fancyError
}

function objcjs_main(a, b){
    makeError()
   
    var JSError = NSError.extend("JSError2")
    NSLog("JSError Class: %@", JSError)
    
    var error = JSError["errorWithDomain:code:userInfo:"]("Javascript", ObjCInt(1337), null)
    NSLog("error code %d", error.code)

    var mySwiftyStyleCreatedError = JSError.errorWithDomainCodeUserInfo("Javascript",
                                                                        ObjCInt(42),
                                                                        null)
    NSLog("mySwiftyStyleCreatedError code %d", mySwiftyStyleCreatedError.code)
    return 0
}
