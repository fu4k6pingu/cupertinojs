function Counter(init){
    this.value = init
    
    function increment(){
        this.value = this.value + 1
    }
    
    this.increment = increment
    return this
}

function objcjs_main(a, b){
    //TODO : all imports must be put here
    // needs pass for macros first
    objc_import("examples/objc-exampleheader.h")

    var c = new Counter(1)
    c.increment(null)
    c.increment(null)
    
    NSLog("NSArray.arrayWithObject(\"bananna\") %@", 
          NSArray
          .arrayWithObject("bananna"))

    NSLog("NSArray.arrayWithObjects(\"bananna\") %@",
          NSArray
          .arrayWithObjects("bananna", "kiwi"))

    NSLog("NSArray.alloc().initWithObject(\"apple\") %@",
          NSArray
          .alloc()
          .initWithObject("apple"))

    NSLog("NSArray.alloc.initWithObject(\"apple\") %@",
          NSArray
          .alloc
          .initWithObject("apple"))

    //NSArrays
    var fruits =  NSArray.arrayWithObjects("bananna", "kiwi")
    var zero = 0
    NSLog("First fruit %@", fruits.objectAtIndex(zero.intValue))

    //Native array
    var fruits =  ["bananna", "kiwi"]
    NSLog("First fruit %@", fruits[0])
    
    //do crazy stuff like this
    //TODO : keyed property assignment
    //fruits[99] = "lemon"
    //NSLog("100th fruit %@", fruits[99])
    return 0
}
