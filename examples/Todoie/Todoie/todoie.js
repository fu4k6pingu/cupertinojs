objc_import("imports.h")


function TodoieAppDelegate(){
    function _init(){
        NSLog("Init!")
        return this
    }
    
    function didFinish(){
        NSLog("Init!")
    }
    
    this.init = _init;
    this.applicationDidFinishLaunchingWithOptions = didFinish
    
    return todoie
}

function run(){
    new TodoieAppDelegate()
}