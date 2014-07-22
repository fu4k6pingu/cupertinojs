function larryf(argument){
    var i;
    for (i = 0; i < argument; i = i + 1){
        if (i == 5){
            continue;
        }
        
        NSLog("for: %@", i);
    }
    
//    var j = 0;
//   
//    while (j < 10) {
//        j = j + 1
//        if (j == 5)
//            continue
//        
//        NSLog("do: %@", j);
//    }
   
    return 0 
}

function objcjs_main(a, b){
    NSLog("input 10: %@", larryf(10))
    return 0
}
