function console(identifer){
    this.identifer = identifer
    NSLog("init console %@", this.identifer);

    function log(arg){
        NSLog("LOG: %@: %@", this.identifer, arg);

        var that = this

        function toString(){
            return that.identifer
        }
    
        this.toString = toString
    }
    
    this.log = log 
}

function objcjs_main(a, b){
    var c = new console.log();
    NSLog("c %@", c)
//    NSLog("c identifer %@", c.identifer)
//    NSLog("c.log().toString %@", c.log().toString())
    return 0
}
