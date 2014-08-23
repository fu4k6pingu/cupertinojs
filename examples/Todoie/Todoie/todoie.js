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

function OnLoad(){
    var TodoListViewController = UIViewController.extend("TodoListViewController")
    
    TodoListViewController.prototype.loadView = function(){
        //A javascript array of fruit
        this.fruits = ["apples", "banannas", "oranges"]

        var tableView = UITableView.alloc.init
        tableView.registerClassForCellReuseIdentifier(UITableViewCell.class, "FruitCell")
        tableView.delegate = this
        tableView.dataSource = this
        tableView.tableFooterView = UIView.new
        tableView.alwaysBouncesVertical = 1
        this.view = tableView
    }
    
    TodoListViewController.prototype.dealloc = function(){
        this.view.dataSource = null;
        this.view.delegate = null;
    }
    
    // UITableViewDataSource
    TodoListViewController.prototype.tableViewNumberOfRowsInSection = function(tableView, section){
        //FIXME: implement array length
        return ObjCInt(3)
    }
    
    TodoListViewController.prototype.tableViewDidSelectRowAtIndexPath = function(tableView, indexPath){
        var fruit = this.fruits[Number(indexPath.row)]
        UIAlertView
        .alloc
        .initWithTitleMessageDelegateCancelButtonTitleOtherButtonTitles("Don't forget", fruit, null, "Got it", null)
        .show
    }
    
    TodoListViewController.prototype.numberOfSectionsInTableView = function(tableView){
        return ObjCInt(1)
    }
    
    TodoListViewController.prototype.tableViewCellForRowAtIndexPath = function(tableView, indexPath){
        var cell = tableView.dequeueReusableCellWithIdentifier("FruitCell")
        
        //A JS array is indexed by objects - cast the integer indexPath.row to a number
        cell.textLabel.text = this.fruits[Number(indexPath.row)]
        return cell
    }
    
    // Declare a function DidFinishLaunching
    function DidFinishLaunching(application, options){
        // Structs are accessed with the cast constructor
        var applicationFrame = CGRect(UIScreen.mainScreen.bounds)
        
        // Once structs are casted to objects, they work just like regular JS objects
        NSLog("height %@", applicationFrame.size.height)
        
        var window = UIWindow.alloc.init
        window.frame = applicationFrame
        window.makeKeyAndVisible()
        this.window = window
        
        var todoListController = TodoListViewController
                                    .alloc
                                    .init
        todoListController.title = "Shopping list"
        
        window.rootViewController = UINavigationController
                                        .alloc
                                        .initWithRootViewController(todoListController)
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

