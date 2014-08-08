// objc_import - a macro similar to #import which
// can import Objective-C headers with search path being SRC_ROOT
//
// Classes and selectors are exported to the global namespace
objc_import("imports.h")

// Similar to swift - Objective C selectors are converted
// ex +[NSNumber numberWithInt:int]
// is the same as NSNumber.numberWithInt(int)
// it is also possible to invoke selectors with
// thier objc name ex NSNumber["numberWithInt:"](int)
function Number(int){
    return NSNumber.numberWithInt(int)
}

//JS numbers are implemented as NSNumbers
function ObjCInt(int){
    return int.intValue
}

// Structs are wrapped in JS Objects and can be accessed
// with category methods ex -UIView.initWithFrameValue
function Rect(x, y, width, height){
    return {
        "x" : x,
        "y" : y,
        "width" : width,
        "height" : height
    }
}

function OnLoad(){
    function DidFinishLaunching(application, options){
        //Initialize the app delegate
        var appFrame = Rect(0, 0, 320, 480)

        var window = UIWindow.alloc.initWithFrameValue(appFrame)
        window.makeKeyAndVisible()
        
        // assigning a property with a value
        // will invoke the setter on the receiver
        this.window = window
       
        this.fruits = ["apples", "banannas", "oranges"]
        
        var tableView = UITableView.alloc.initWithFrameValue(appFrame)
        tableView.registerClassForCellReuseIdentifier(UITableViewCell.class, "FruitCell")
        tableView.delegate = this
        tableView.dataSource = this
        tableView.separatorColor = UIColor.clearColor
        
        var header = UILabel.alloc.initWithFrameValue(Rect(0, 0, 320, 50))
        header.text = "Shopping list"
        header.setTextAlignment(ObjCInt(1))
        tableView.tableHeaderView = header
        
        this.tableView = tableView

        window.addSubview(tableView)
    }

    function AppDelegate(){}

    // Implementing a method can be achived by
    // assigning a function to the property of a JSFunction or ObjC class
    //
    // it can also be achieved by assigning properties to 'this'
    // best practice is to use prototypes or static initializers
    // because assigning a function to a property causes the method
    // to be implemented
    
    AppDelegate.prototype.applicationDidFinishLaunchingWithOptions = DidFinishLaunching

    AppDelegate.prototype.dealloc = function(){
        this.tableView.dataSource = null;
        this.tableView.delegate = null;
    }
   
    // UITableViewDataSource
    AppDelegate.prototype.tableViewNumberOfRowsInSection = function(tableView, section){
        return ObjCInt(3)
    }
    
    AppDelegate.prototype.numberOfSectionsInTableView = function(tableView){
        return ObjCInt(1)
    }
    
    AppDelegate.prototype.tableViewCellForRowAtIndexPath = function(tableView, indexPath){
        var cell = tableView.dequeueReusableCellWithIdentifier("FruitCell")
        //A JS array is indexed by objects
        var fruit = this.fruits[Number(indexPath.row)]
        var textLabel = cell.textLabel
        cell.textLabel.text = fruit
        return cell
    }
}

//The main function (invoked by the os)
function main(argc, argv){
    OnLoad()

    //A wrapper for UIApplicationMain with a autorelease pool
    return UIApplication.applicationMainArgVName(argc, argv, "AppDelegate")
}
