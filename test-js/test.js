function larryf(argument){
    var i;
    for (i = 0; i < argument; i = i + 1){
        if (i == 5){
            continue;
        }
        
        NSLog("%@", i);
    }
}

function objcjs_main(a, b){
    NSLog("input 10: %@", larryf(10))
    return 0
}
