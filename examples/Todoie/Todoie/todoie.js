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
    var TodoListViewController = UIViewController.extend("TodoListViewController")
   
    TodoListViewController.prototype.viewDidLoad = function(){
        this.setValueForKey(0, "automaticallyAdjustsScrollViewInsets")
    }
    
    TodoListViewController.prototype.loadView = function(){
        this.fruits = ["apples", "banannas", "oranges"]
        
        var tableView = UITableView.alloc.initWithFrameValue(Rect(0, 0, 320, 480))
       
        tableView.registerClassForCellReuseIdentifier(UITableViewCell.class, "FruitCell")
        tableView.delegate = this
        tableView.dataSource = this
        tableView.separatorColor = UIColor.clearColor
        
        this.view = tableView
    }
    
    TodoListViewController.prototype.dealloc = function(){
        this.view.dataSource = null;
        this.view.delegate = null;
    }
    
    // UITableViewDataSource
    TodoListViewController.prototype.tableViewNumberOfRowsInSection = function(tableView, section){
        return ObjCInt(3)
    }
    
    TodoListViewController.prototype.numberOfSectionsInTableView = function(tableView){
        return ObjCInt(1)
    }
    
    TodoListViewController.prototype.tableViewCellForRowAtIndexPath = function(tableView, indexPath){
        var cell = tableView.dequeueReusableCellWithIdentifier("FruitCell")
        //A JS array is indexed by objects
        var fruit = this.fruits[Number(indexPath.row)]
        var textLabel = cell.textLabel
        cell.textLabel.text = fruit
        return cell
    }
    
    function DidFinishLaunching(application, options){
        //Initialize the app delegate

        // assigning a property with a value
        // will invoke the setter on the receiver
        var window = UIWindow.alloc.initWithFrameValue(Rect(0, 0, 320, 480))
        this.window = window.retain

        window.rootViewController = UINavigationController.alloc.initWithRootViewController(TodoListViewController.alloc.init)
        window.makeKeyAndVisible()
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
}

//The main function (invoked by the os)
function main(argc, argv){
    OnLoad()

    //A wrapper for UIApplicationMain with a autorelease pool
    return UIApplication.applicationMainArgVName(argc, argv, "AppDelegate")
}
