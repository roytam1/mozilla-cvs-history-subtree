class BaseClass {
    
    static var gBC = 12;

    operator function "+" (a:BaseClass, b:Number) 
    {    
        gBC++;
        return a.mBC + b;
    }


    var mBC;

    function fBC(a)
    {
        return mBC + a;
    }

    function get Q() { return 13; }
    function set Q(a) { gBC = 99 + a; }

    constructor function makeBC()
    {
        mBC = 17;
    }

}

var result = true;

var bc:BaseClass = BaseClass.makeBC();

if (BaseClass.gBC != 12) result = false;
BaseClass.gBC = 0;

if ((bc + 3) != 20) result = false;
if (BaseClass.gBC != 1) result = false;

class Extended extends BaseClass {

    var mEx;
    var t = 49200;

    function Extended() 
    {
        mEx = 2;
    }

    operator function "+" (a:Extended, b:BaseClass)
    {
        return a.mEx + b.mBC;
    }


}

var ex:Extended = new Extended;
if (ex.t != 49200) result = false;
if ((ex + bc) != 19) result = false;

function loopy(a)
{
    var x = 0;
    foo: 
        while (a > 0) {            
            --a;
            x++;
            if (x == 3) break foo;
        }
        
    return x;
}

if (loopy(17) != 3) result = false;

BaseClass.gBC *= ex.mEx;
if (BaseClass.gBC != 2) result = false;

var a = 3, b = 2;
a &&= b;
if (a != 2) result = false;

if (bc.Q != 13) result = false;
bc.Q = 1;
if (BaseClass.gBC != 100) result = false;

if (result) print("still sane") else print("gone off the deep end");
