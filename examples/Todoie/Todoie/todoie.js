objc_import("imports.h")

function Rect(x, y, width, height){
    return {
        "x" : x,
        "y" : y,
        "width" : width,
        "height" : height
    }
}

function ObjCInt(int){
    return int.intValue
}

function Number(int){
    return NSNumber.numberWithInt(int)
}

function documentLoaded(){
    function TodoieAppDelegate(){}
  
    function numberOfRows(section, a){
        return ObjCInt(3)
    }
    
    function numberOfSections(){
        return ObjCInt(1)
    }
   
    function tableViewCellForRowAtIndexPath(tableView, indexPath){
        var cell = tableView.dequeueReusableCellWithIdentifier("FruitCell")
        var fruit = this.fruits[Number(indexPath.row)]
        var textLabel = cell.textLabel
        cell.textLabel.text = fruit
        cell.backgroundColor = UIColor.redColor
        return cell
    }
    
    function didFinish(a, b){
        var appFrame = Rect(0, 0, 320, 480)

        var window = UIWindow.alloc.initWithFrameValue(appFrame)
        window.makeKeyAndVisible()
        this.window = window

        var tableView = UITableView.alloc.initWithFrameValue(appFrame)
        tableView.registerClassForCellReuseIdentifier(UITableViewCell.class, "FruitCell")
      
        this.tableViewNumberOfRowsInSection = numberOfRows
        this.numberOfSectionsInTableView = numberOfSections
        this.tableViewCellForRowAtIndexPath = tableViewCellForRowAtIndexPath
        window.addSubview(tableView)
        this.fruits = ["apples", "banannas", "oranges"]
        
        tableView.delegate = this
        tableView.dataSource = this
    }

    function TodoieAppDelegateInit(){
        NSLog("Init!")
        this.applicationDidFinishLaunchingWithOptions = didFinish
        return this
    }
    
    TodoieAppDelegate.prototype.init = TodoieAppDelegateInit
}

function main(a, b){
    documentLoaded()
    return UIApplication.applicationMainArgVName(a, b, "TodoieAppDelegate")
}

