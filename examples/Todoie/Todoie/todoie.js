objc_import("imports.h")


function documentLoaded(){
    NSLog("Document loaded")
    
    function TodoieAppDelegate(){}
    
    function didFinish(a, b){
        NSLog("Did finish launching!")
    }

//    TodoieAppDelegate.prototype["application:didFinishLaunchingWithOptions"] = didFinish
    TodoieAppDelegate.prototype.applicationDidFinishLaunchingWithOptions = didFinish

    function TodoieAppDelegateInit(){
        NSLog("Init!")
        return this
    }
    TodoieAppDelegate.prototype.init = TodoieAppDelegateInit
}

function main(a, b){
    documentLoaded()
    return NSObject.applicationMainArgVName(a, b, "TodoieAppDelegate")
}

