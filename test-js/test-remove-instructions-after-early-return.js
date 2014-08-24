objc_import("test-imports.h")

function DidLoad(){
    var color = 1
    return

    //This should be removed
    NSLog("No color")
}

function main(a,b){
    DidLoad()
    var zero = 0
    return zero.intValue
}