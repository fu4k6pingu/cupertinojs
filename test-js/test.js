function Platform(version){
    this.userInfo = {
        "_version" : version,
        "identifier" : "OBJCJS",
        "userInfo" : {
            "APIKey" : 42
        },
    };
    
    this.name = "turtle neck"
}

function objcjs_main(a, b){
    var P = Platform
    var platform = new P(.1)
    NSLog("platform %@", platform)
    NSLog("name %@", platform.userInfo)
    NSLog("version %@", platform.userInfo._version)
    NSLog("APIKey %@", platform.userInfo.userInfo.APIKey)
    return 0
}

