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

var badTest = "";
var test = 1;

var bc:BaseClass = BaseClass.makeBC();

if (BaseClass.gBC != 12) badTest += test + " ";
BaseClass.gBC = 0;

test++;
if ((bc + 3) != 20) badTest += test + " ";
test++;
if (BaseClass.gBC != 1) badTest += test + " ";

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
test++;
if (ex.t != 49200) badTest += test + " ";
test++;
if ((ex + bc) != 19) badTest += test + " ";

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

test++;
if (loopy(17) != 3) badTest += test + " ";

BaseClass.gBC *= ex.mEx;
test++;
if (BaseClass.gBC != 2) badTest += test + " ";

var a = 3, b = 2;
a &&= b;
test++;
if (a != 2) badTest += test + " ";

test++;
if (bc.Q != 13) badTest += test + " ";
bc.Q = 1;
test++;
if (BaseClass.gBC != 100) badTest += test + " ";


var cnX = 'X'
var s = '';
function f(n) { var ret = ''; for (var i = 0; i < n; i++) { ret += cnX; } return ret; }
s = f(5);
test++;
if (s != 'XXXXX') badTest += test + " ";

var t = "abcdeXXXXXghij";
test++;
if (t.split('XXXXX').length != 2) badTest += test + " ";


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
        test++;
        badTest += test + " ";
    }
    catch (e) {
        test++;
        if (a != 12) badTest += test + " ";
        test++;
        if (b != c / 5) badTest += test + " ";
        test++;
        if (e != "frisbee") badTest += test + " ";
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
test++;
if (sw(2) != "02d") badTest += test + " ";


class A { static var x = 2; var y; }
class B extends A { static var i = 4; var j; }

test++;
if (A.x != 2) badTest += test + " ";

test++;
if (B.x != 2) badTest += test + " ";

test++;
if (B.i != 4) badTest += test + " ";





if (badTest == 0) print("still sane") else print("gone off the deep end at test #" + badTest);
