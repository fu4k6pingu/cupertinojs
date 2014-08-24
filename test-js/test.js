//<expect>bullitburbon

objc_import("test-imports.h")

function DidLoad(){
    var boozey = true

    NSLog("bullitburbon")
    return
    
    if (1 < 2){
        NSLog("bullitburbon")
    }
}

function main(a,b){
    DidLoad()
    var zero = 0
    return zero.intValue
}