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
    var JSCountedError = NSError.extend("JSCountedError")
    var counter = new Counter(0)

    function CountedErrorFactory(domain, code, userInfo){
        counter.increment(0)
        NSLog("CountedErrorFactory errors created %@", counter.value)
        
        return NSError.errorWithDomainCodeUserInfo(domain,
                                                   code,
                                                   userInfo)
    }
  
    //Use fancy selector name conversion
    JSCountedError.errorWithDomainCodeUserInfo = CountedErrorFactory

    //which is equivilant to this
    // JSCountedError["errorWithDomain:code:userInfo:"] = CountedErrorFactory

    NSLog("CountedErrorFactory - parent %@", CountedErrorFactory._objcjs_parent);
    
    var fancyError = JSCountedError.errorWithDomainCodeUserInfo("Too much whisky!",
                                                                ObjCInt(1984),
                                                                null)
    NSLog("fancyError %@", fancyError)
    
    return fancyError
}

function main(a, b){
    makeError()
   
    var JSError = NSError.extend("JSError2")
    NSLog("JSError Class: %@", JSError)
    
    var error = JSError["errorWithDomain:code:userInfo:"]("Javascript", ObjCInt(1337), null)
    NSLog("error code %d", error.code)

    var mySwiftyStyleCreatedError = JSError.errorWithDomainCodeUserInfo("Javascript",
                                                                        ObjCInt(42),
                                                                        null)
    NSLog("mySwiftyStyleCreatedError code %d", mySwiftyStyleCreatedError.code)
    
    return ObjCInt(0)
}
