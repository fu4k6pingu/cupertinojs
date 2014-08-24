function Platform(){
    this.userInfo = {
        "identifier" : "CUJS",
        "userInfo" : {
            "APIKey" : 42
        },
        2 : 2,
        "4" : 4,
    };

    this.name = "turtle neck"
}

function main(a, b){
    var P = Platform
    var platform = new P()

    NSLog("platform %@", platform)
    
    platform["updatedAt"] = "Monday"
    platform.createdAt = "Sunday"

    NSLog("platform.updatedAt %@", platform.updatedAt)
    NSLog("platform[\"updatedAt\"] %@", platform["updatedAt"])
    NSLog("platform.createdAt %@", platform.createdAt)
    NSLog("platform[\"createdAt\"] %@", platform["createdAt"])
    

    NSLog("NAMED - NAMED")
    NSLog("name %@", platform.name)
    NSLog("userInfo %@", platform.userInfo)

    NSLog("NAMED - KEYED")
    NSLog("name %@", platform["name"])
    NSLog("userInfo %@", platform["userInfo"])

    NSLog("platform[\"userInfo\"].identifier %@", platform["userInfo"].identifier)
    
    
    NSLog("KEYED - NAMED")
    NSLog("platform.userInfo %@", platform.userInfo)
    platform.userInfo.identifier
    NSLog("platform.userInfo.identifier %@", platform.userInfo.identifier)
    NSLog("platform.userInfo[\"userInfo\"].APIKey %@", platform.userInfo["userInfo"].APIKey)

    var userInfo = platform.userInfo
    NSLog("userInfo.identifier %@", userInfo.identifier)
    NSLog("userInfo[\"identifier\"] %@", userInfo["identifier"])
    
    NSLog("KEYED - KEYED")
    NSLog("platform.userInfo[\"identifier\"] %@", platform.userInfo["identifier"])
    NSLog("platform.userInfo[\"userInfo\"][\"APIKey\"] %@", platform.userInfo["userInfo"]["APIKey"])

    NSLog("NAMED - NUMBER-KEYED")
    NSLog("platform.userInfo[2] %@", platform.userInfo[2])
    NSLog("platform.userInfo[4] %@", platform.userInfo[4])

    NSLog("NAMED - NUMBER-KEYED")
    NSLog("platform.userInfo[\"2\"] %@", platform.userInfo["2"])
    NSLog("platform.userInfo[\"4\"] %@", platform.userInfo["4"])
    
    platform.userInfo.status = "!!~~BIM~~!!"
    NSLog("platform.userInfo.status %@", platform.userInfo.status)
    
    platform["userInfo"].status = "!!~~BOOMM~~!!"
    NSLog("platform[\"userInfo\"].status %@", platform["userInfo"].status)

    var four = "4"
    NSLog("platform.userInfo[four] (\"4\")%@", platform.userInfo[four])

    var four = 4
    NSLog("platform.userInfo[four] (4)%@", platform.userInfo[four])
   
    P.console = "console"
    NSLog("P[\"console\"] %@", P["console"])

    //TODO :
//    P[0] = "__first"
//    NSLog("P[0] %@", P[0])
    
    var fruits = ["apples", "banannas"]
    NSLog("fruits %@", fruits)
    NSLog("fruits %@", fruits[0])
    return 0
}
