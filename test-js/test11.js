// This takes ~14.5 seconds on my hac pro in chrome

//var endValue = 10000000;
//var a = Date.now();
//var a3 = 10;
//var b3 = 10;
//
//for (var i = 1; i < endValue; i = i +1 ) {
//    a3 += b3;
//};
//
//console.log((a - Date.now())/ 1000)

function testPlusEqual(a, b){
    for (var i = 0; i < 10000000; i++)
        a += b
    return a;
}

function testMinusEqual(a, b){
    var result = a -= b
    NSLog("%@", result);
}

function testBitOr(a, b){
    return a | b
}

function testBitXOr(a, b){
    return a ^ b
}

function testBitAnd(a, b){
    return a & b
}

function testBitShiftLeft(a, b){
    return a << b
}

function testBitShiftRight(a, b){
    return a >> b
}

function testBitShiftRightRight(a, b){
    return a >>> b
}

function testAssignBitShiftRightRight (a, b){
    a >>>= b;
    return a
}

function runtests(){
    NSLog("bitor %@", testBitOr(10, 42))
    NSLog("testBitXOr %@", testBitXOr(10, 42))
    NSLog("testBitAnd %@", testBitAnd(10, 42))
    NSLog("testBitShiftLeft %@", testBitShiftLeft(10, 42))
    NSLog("bitShiftRight %@", testBitShiftRight(10, 42))
    NSLog("bitShiftRightRight %@", testBitShiftRightRight(10, 1))

    NSLog("assignBitShiftRightRight  %@", testAssignBitShiftRightRight(10, 1))
}

function main(a, b){
    runtests()
    return 0
}
