# Cupertino.js - Compile Javascript to Cocoa

Cupertino.js is a implementation of the Javascript programming language designed
with Cocoa support as a primary consideration. It includes a static compiler and
_uber dynamic_ runtime that sits atop of the Objective-C runtime. 

It exposes a simple interface to Objective-C: Javascript code _is_ statically
compiled to Objective-C. Objects and functions are derived from NSObject,
strings are NSStrings, numbers are NSNumbers, and message dispatch uses
``objc_msgSend``.

### Declare variables
```javascript
var name = "Steve"
```

### Perform arithmetic
```javascript
var seven = 4 + 3
```

### Loop 
```javascript
for (var i = 99; i > 0; i--){
    NSLog("%@ bottles of beer on the wall.", i) 
}
```

### Objects
```javascript
var tallBoy = {"label" : "Pabst", "size" : 24.0}
```

### Define functions, methods, and properties
```javascript
function Todo(){
    this.message = null

    this.log = function(){
        NSLog("%@", this.message)
    }
}
```

### Instantiate functions
```javascript
var beers = new Todo();
beers.message = "Drink some beers"
```

### Call functions 
```javascript
beers.log()
```

### Call Objective-C methods
```javascript
var bottlesOnTheWall = NSString.stringWithFormat("%@ of beer on the wall", 99)
bottlesOnTheWall.lowercaseString()

//short hand
bottlesOnTheWall.lowercaseString
```

### Message send struct
```javascript
//Structs are casted from c structs to Javascript objects
//cast functions take the struct as argument and are named after the type or typedef
var applicationFrame = CGRect(UIScreen.mainScreen.applicationFrame)

NSLog("screen height %@", applicationFrame.size.height)
```

### Mutate structs
```javascript
applicationFrame.origin.y = 10
```

### Pass structs back to Objective-C types
```javascript
var window = UIWindow.alloc.init
window.frame = applicationFrame
```

### Define class properties
```javascript
Todo.entityName = "Todo"
```

### Full support for 'this'
```javascript
this.fruits = ["apples", "banannas", "oranges"]
```

### Subclass Objective-C classes
```javascript
var TodoListController = UIViewController.extend("TodoListController")
```

### Prototype instance methods
```javascript
TodoListViewController.prototype.numberOfSectionsInTableView = function(tableView){
    return ObjCInt(1)
}
```

### Bind function objects as methods
```javascript
// Define function AppDelegate
function AppDelegate(){}

function DidLaunch(application, options){
    NSLog("Launched! %@", this)
}

// Implement  -[AppDelegate application:didFinishLaunchingWithOptions:]
// with the body of DidLaunch
//
// 'this' is an instance of AppDelegate
AppDelegate.prototype.applicationDidFinishLaunchingWithOptions = DidLaunch
```

### Import Objective-C
```javascript
//Use the objc_import macro to import objective-c headers
objc_import("TDNetworkRequest.h")

// Interfaces are exported to the global namespace
var request = TDNetworkRequest.get("latest-todos")
reqest.send()

```

## Examples

The examples directory contains a iPhone app that runs in the simulator
and comes complete with a UITableView implementation. 

The test-js directory includes JS snippets


## State of union:

Although the example app builds and runs in the simulator, Cupertino.js is not
yet production ready.

There several features not yet implemented including:

- Garbage collection: currently it uses retain/release/autorelease
- Standard JS lib (Strings, arrays, etc)
- Objective-C super
- Switch statements
- JS modules
- JS repl
- ECMA standard

## Project setup

Cupertino.js has the following dependencies:

- v8 
- llvm
- libclang
- Xcode
- Foundation & the Objective-C runtime

There is an install script which sets up the development environment

### Script install

    ./bootstrap.sh




