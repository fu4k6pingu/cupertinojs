
function one(){
    NSLog("in one");
    return 1
}

function testBinaryAndStatment1(a){
    var b = (a && one());
    if (b == 100000){
        NSLog("false");
    } else {
        NSLog("value: %@", b);
    }
}

function testBinaryAndStatment2(a){
    var b = (one() && a);
    if (b == 100000){
        NSLog("false");
    } else {
        NSLog("value: %@", b);
    }
}

function testBinaryOrStatment1(a){
    var b = (a || one());
    if (b == 100000){
        NSLog("false");
    } else {
        NSLog("value: %@", b);
    }
}

function testBinaryOrStatment2(a){
    var b = (one() || a);
    if (b == 100000){
        NSLog("false");
    } else {
        NSLog("value: %@", b);
    }
}

function runtests(){
    testBinaryAndStatment1(0)
    testBinaryAndStatment1("sally")

    testBinaryAndStatment2(0)
    testBinaryAndStatment2("sally")

    testBinaryOrStatment1(0)
    testBinaryOrStatment1("sally")
    
    testBinaryOrStatment2(0)
    testBinaryOrStatment2("sally")
}

function objcjs_main(a, b){
    runtests()
    return 0
}
