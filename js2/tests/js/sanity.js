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


var cnX = 'X'
var s = '';
function f(n) { var ret = ''; for (var i = 0; i < n; i++) { ret += cnX; } return ret; }
s = f(5);
if (s != 'XXXXX') result = false;

var t = "abcdeXXXXXghij";
if (t.split('XXXXX').length != 2) result = false;


function x()
{
    var a = 44; 
    throw('frisbee');
}

function ZZ(b)
{
    var a = 12;
    var c = b * 5;
    try {
        x();
        result = false;
    }
    catch (e) {
        if (a != 12) result = false;
        if (b != c / 5) result = false;
        if (e != "frisbee") result = false;
    }
}
ZZ(6);

function sw(t) 
{
    var result = "0";
    switch (t) {

        case 1:
            result += "1";
        case 2:
            result += "2";
        default:
            result += "d";
            break;
        case 4:
            result += "4";
            break;
    }
    return result;
}
if (sw(2) != "02d") result = false;


if (result) print("still sane") else print("gone off the deep end");
