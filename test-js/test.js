//objc_import("examples/objc-exampleheader.h")

function Counter(init){
    this.value = init
    this.increment = function(){
        this.value = this.value + 1
    }

    NSLog("foo")
    return this
}

function makeRun(){
    var c = new Counter(9)
    c.increment()
    return this
}


function main(a,b){
    makeRun()
    return 0
}