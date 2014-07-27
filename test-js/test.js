var aGlobal = "EpicGlobal"

function log(value){
    //aGlobal is changed, but not the value that is passed in
    aGlobal = "changed"
    
    //so this will log the value (EpicGlobal)
    NSLog("%@: %@", this, value)

    //this will log the new value
    NSLog("aGlobal locally %@", aGlobal)
}

function objcjs_main(a, b){
    log(aGlobal)
   
    //this should log the changed value
    NSLog("aGlobal end %@", aGlobal)
    return 0
}
