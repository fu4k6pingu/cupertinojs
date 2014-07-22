function testForBreak(){
    for (var i = 0; i < 10; i = i + 1){
        if (i == 5)
            break
        
        NSLog("for: %@", i);
    }

    NSLog("after: %@", i);
}

function testForContinue(){
    for (var i = 0; i < 10; i = i + 1){
        if (i == 5)
            continue
        
        NSLog("for: %@", i);
    }

    NSLog("after: %@", i);
}

function testWhileBreak(){
    var j = 0;
    while (j < 10) {
        j = j + 1
        if (j == 5)
            break
            
        NSLog("do: %@", j);
    }
    
    NSLog("after: %@", j);
}

function testWhileContinue(){
    var j = 0;
    while (j < 10) {
        j = j + 1
        if (j == 5)
            continue
            
        NSLog("while: %@", j);
    }
    
    NSLog("after: %@", j);
}

function testDoWhileBreak(){
    var j = 0;
    do {
        j = j + 1
        if (j == 5)
            continue
            
        NSLog("while: %@", j);
    } while (j < 10)
    
    NSLog("after: %@", j);
}

function testDoWhileContinue(){
    var j = 0;
    do {
        j = j + 1
        if (j == 5)
            continue
            
        NSLog("do: %@", j);
    } while (j < 10)
    
    NSLog("after: %@", j);
}

function objcjs_main(a, b){
    testForBreak()
    testForContinue()
    testDoWhileBreak()
    testDoWhileContinue()
    testWhileBreak()
    testWhileContinue()
    return 0
}
