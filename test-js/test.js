var aGlobal = "EpicGlobal"

function log(value){
    //aGlobal is changed, but not the value that is passed in
    aGlobal = "changed"
    
    //so this will log the value (EpicGlobal)
    NSLog("%@: %@", this, value)

    //this will log the new value
    NSLog("aGlobal locally %@", aGlobal)
}

function doMain(){
    log(aGlobal)
  
    //this should log the changed value
    NSLog("aGlobal after log: %@", aGlobal)
   
    aGlobal = "local"
    
    function inside(){
        //this should log the "local" value
        NSLog("aGlobal inside: %@", aGlobal)
    }
    
    inside()
    
    NSLog("aGlobal end: %@", aGlobal)
}

function objcjs_main(a, b){
    doMain()
    return 0
}

