function larryf(argument){
    var i;
    for (i = 0; i < argument; i = i + 1){
        if (i < 5){
            if (argument == 4){
                return 4
            } else{
                NSLog("Argument is not 4", i)
            }
        } else {
            NSLog("%@ is not less than 5 and does not equal 4", i)
            if (i == 9){
                return 9
            }
        }
    }
    return 666;
}

function larryf2(argument, argument2){
    if (argument == argument2){
//        return 0
    } else if( argument < argument2){
//        return -1
    } else {
//        return 1
    }
    
    if (argument == argument2){
        NSLog("equal");
    } else if( argument < argument2){
        NSLog("less than");
    } else {
        NSLog("greater than");
    }
    
//    return 11
}

function larryf3(argument, argument2){
    if (argument == argument2){
        NSLog("equal");
        return 22;
    } else if( argument < argument2){
        NSLog("less than");
        return 22;
    } else {
        NSLog("greater than");
    }
    
    if (argument == argument2){
        return 0
    } else if( argument < argument2){
        return -1
    } else {
        return 1
    }
    
    return 0
}
function objcjs_main(a, b){
    NSLog("input 1,2: %@", larryf2(1, 2))
    NSLog("input 2,1: %@", larryf2(2, 1))
    NSLog("input 10: %@", larryf(10))
    NSLog("input 1: %@", larryf(4))
    return 0
}
