function console(identifer){
    this.identifer = identifer
    NSLog("init console %@", this.identifer);

    function log(arg){
        NSLog("%@: %@", this.identifer, arg);
        return 0
    }
    
    this.log = log
}

function objcjs_main(a, b){
    var c = new console(1);
    NSLog("c %@", c)
    c.log("sam");
    return 0
}
