class BaseClass {
    
    static var gBC;

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

    constructor function makeBC()
    {
        mBC = 17;
    }

}

var result = true;

var bc:BaseClass = BaseClass.makeBC();

BaseClass.gBC = 0;

if ((bc + 3) != 20) result = false;
if (BaseClass.gBC != 1) result = false;

class Extended extends BaseClass {

    var mEx;

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

if (result) print("still sane") else print("gone off the deep end");
