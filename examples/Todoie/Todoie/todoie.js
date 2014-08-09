objc_import("imports.h")

function OnLoad(){
    function DidFinishLaunching(application, options){
        var window = UIWindow.alloc.init
        this.window = window
        window.makeKeyAndVisible()
        NSLog("Frame %@", objc_Struct("CGRect", window.frame))
        NSLog("Center %@", objc_Struct("CGPoint", window.center))
   }

    function AppDelegate(){}
    AppDelegate.prototype.applicationDidFinishLaunchingWithOptions = DidFinishLaunching
}

//The main function (invoked by the os)
function main(argc, argv){
    OnLoad()

    //A wrapper for UIApplicationMain with a autorelease pool
    return UIApplication.applicationMainArgVName(argc, argv, "AppDelegate")
}
