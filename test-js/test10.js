function testIncPostFix(b){
    NSLog("value: %@", b++);
    NSLog("value after: %@", b);
}

function testIncPreFix(b){
    NSLog("value: %@", ++b);
    NSLog("value after: %@", b);
}

function testDecPostFix(b){
    NSLog("value: %@", b--);
    NSLog("value after: %@", b);
}

function testDecPreFix(b){
    NSLog("value: %@", --b);
    NSLog("value after: %@", b);
}

function runtests(){
    testIncPostFix(1)
    testIncPreFix(1)
    testDecPostFix(1)
    testDecPreFix(1)
}

function main(a, b){
    runtests()
    return 0
}
