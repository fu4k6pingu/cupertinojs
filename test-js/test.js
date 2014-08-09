//objc_import("examples/objc-exampleheader.h")

function Counter(init){
    this.value = init
    this.increment = function(){
        this.value = this.value + 1
        return this.value
    }

    return this
}

function main(a,b){
    (function(){
         var c = new Counter(9)
         var log = function(value){
            NSLog("incremented %@", value)
         }
         log(c.increment())
        return this
    }())

    var zero = 0
    return zero.intValue
}