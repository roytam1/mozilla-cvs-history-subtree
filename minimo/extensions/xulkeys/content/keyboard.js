/*
 * We observe the keyboard notification ( open || close ) 
 */
 
var xulkeys_keyboardObserver= { 

  observe:function (subj, topic, data) {

   var skeyNotifyOnly = null;
   try {
     skeyNotifyOnly = gPref.getBoolPref("skey.notifyOnly");
   } 
   catch(e) {skeyNotifyOnly=false;}
   
    if(data=="open")  { 
      if (skeyNotifyOnly==true) {
		 document.getElementById("keyboardContainer_xulkeys").setAttribute("hidden","false");      
      }
    } else if(data=="close")  {
      if (skeyNotifyOnly==true) {
          document.getElementById("keyboardContainer_xulkeys").setAttribute("hidden","true");
      }
    }  
  }    
};

try {
  var os = Components.classes["@mozilla.org/observer-service;1"]
                     .getService(nsIObserverService);
                     
  os.addObserver(xulkeys_keyboardObserver,"software-keyboard", false);

} catch (e) {

} 


var keyMap = 1;

function keyCaps(ii) {

	document.getElementById("keymap"+keyMap).setAttribute("hidden","true");
	keyMap = ii;
	document.getElementById("keymap"+keyMap).setAttribute("hidden","false");

}

function keyFlip() {

	document.getElementById("keymap"+keyMap).setAttribute("hidden","true");

      if(keyMap<3) {
		keyMap++;
	} else {
		keyMap=1;
	}

	document.getElementById("keymap"+keyMap).setAttribute("hidden","false");

}
var timeTap = false;

function enableTimer() {

  timeTap = true;

}

var vStoredTap = "";
var accu = -1; 
var colorGreen=255;
var colorRed=0;
var typing = false;
var ccc=0;

function keyTap(vv) {

  if(!typing ) {

	timeAnim();
	accu=0;

      typing=true;

  prevFocused = document.commandDispatcher.focusedElement;

  } 

  if(typing) {

	  if(vv=="1") {
		 accu++;
		 vStoredTap = "1";
	  } 
	
	  if(vv=="2") {

		 accu++;
		 vStoredTap = "2";
	  } 
	
	  if(vv=="3") {
		 accu++;
		 vStoredTap = "3";
	  } 

	  if(vv=="4") {
		 accu++;
		 vStoredTap = "4";
	  } 
	
	  if(vv=="5") {

		 accu++;
		 vStoredTap = "5";
	  } 
	
	  if(vv=="6") {
		 accu++;
		 vStoredTap = "6";
	  } 


	  if(vv=="7") {
		 accu++;
		 vStoredTap = "7";
	  } 
	
	  if(vv=="8") {

		 accu++;
		 vStoredTap = "8";
	  } 

	  if(vv=="9") {
		 accu++;
		 vStoredTap = "9";
	  } 

  } 



}


function timeAnim() {



  if(ccc <200) {
	   colorRed++;
	   colorGreen--;
	   ccc++;
	   document.getElementById("timetap").style.backgroundColor="rgb("+colorRed+","+colorGreen+",0)";
	 
	   setTimeout("timeAnim()",10);

  } else {

 colorGreen=255;
 colorRed=0;
 ccc = 0;
	   document.getElementById("timetap").style.backgroundColor="rgb("+0+","+255+",0)";

   timeOver(); 

  } 

}

function timeOver() {

      execTap();

}

function execTap() {


  vForward = 0;

  var e = document.createEvent("KeyEvents");
  vCode = 0; 

  vShiftState = false;

  if(vStoredTap=="2") {

	  if(accu==1) vForward = e.DOM_VK_A;
	  if(accu==2) vForward = e.DOM_VK_B;
	  if(accu==3) vForward = e.DOM_VK_C;
  } 

  if(vStoredTap=="3") {

	  if(accu==1) vForward = e.DOM_VK_D;
	  if(accu==2) vForward = e.DOM_VK_E;
	  if(accu==3) vForward = e.DOM_VK_F;

  } 

  if(vStoredTap=="4") {

	  if(accu==1) vForward = e.DOM_VK_G;
	  if(accu==2) vForward = e.DOM_VK_H;
	  if(accu==3) vForward = e.DOM_VK_I;

  } 
 
  if(vStoredTap=="5") {

	  if(accu==1) vForward = e.DOM_VK_J;
	  if(accu==2) vForward = e.DOM_VK_K;
	  if(accu==3) vForward = e.DOM_VK_L;

  } 

  if(vStoredTap=="6") {

	  if(accu==1) vForward = e.DOM_VK_M;
	  if(accu==2) vForward = e.DOM_VK_N;
	  if(accu==3) vForward = e.DOM_VK_O;

  } 

  if(vStoredTap=="7") {

	  if(accu==1) vForward = e.DOM_VK_P;
	  if(accu==2) vForward = e.DOM_VK_Q;
	  if(accu==3) vForward = e.DOM_VK_R;

  } 
  if(vStoredTap=="8") {

	  if(accu==1) vForward = e.DOM_VK_P;
	  if(accu==2) vForward = e.DOM_VK_Q;
	  if(accu==3) vForward = e.DOM_VK_R;

  } 

  if(vStoredTap=="9") {

	  if(accu==1) vForward = e.DOM_VK_W;
	  if(accu==2) vForward = e.DOM_VK_X;
	  if(accu==3) vForward = e.DOM_VK_Y;

  } 
  e.initKeyEvent("keypress",true,true,null,false,false, vShiftState,false, vCode, vForward );

  prevFocused.dispatchEvent(e);

  accu=-1;

  typing=false;

}




var prevFocused = null;

function keyDispatch(vv,state) {

  prevFocused = document.commandDispatcher.focusedElement;

  vForward = 0;

  var e = document.createEvent("KeyEvents");
  vCode = 0; 

  vShiftState = false;
  
  if(state==1) {
	  if(vv=="enter") vCode = e.DOM_VK_ENTER;
	  if(vv=="back") vCode = e.DOM_VK_BACK_SPACE;
	  if(vv=="tab") vCode = e.DOM_VK_TAB;

  } else {
	var charCode = vv.charCodeAt(0);
	if(charCode>=32&&charCode<=126) {
		vForward = charCode;
	}

  } 
  e.initKeyEvent("keypress",true,true,null,false,false, vShiftState,false, vCode,vForward );
  prevFocused.dispatchEvent(e);



}



function dispatchTest() {


  var e = document.createEvent("KeyEvents");

  prevFocused = document.commandDispatcher.focusedElement;

  e.initKeyEvent("keypress",true,true,null,false,false, false,false, 130,0 );

  prevFocused.dispatchEvent(e);

}

function keyboardEventCreate(kk,cc) {

  var e = document.createEvent("KeyEvents");
  e.initKeyEvent("keypress",true,true,null,false,false,true,false, 0,e.DOM_VK_A);


  prevFocused.dispatchEvent(e);


}


function keyboardIntercept_init() {

window.addEventListener("keydown",testGet2,true);

}

function testGet2(e) {

debug(e.keyCode);

}

function testGet(e) {

 e.stopPropagation();

 debug(" will dispatch " + e.keyCode +"  and " + e.charCode);


 window.removeEventListener("keypress",testGet,true);

 keyboardEventCreate(e.keyCode,e.charCode);


}



function debug(ss) {
       document.getElementById("debug").innerHTML=ss;
}

function initKeyboard() {

       document.getElementById('q1').addEventListener("mousedown",hq1,true);

       //keyboardIntercept_init();

       document.getElementById("source").focus();

 keyboardEventCreate(0,113);


}

function hq1(e) {

 e.stopPropagation();


 keyboardEventCreate(0,113);


}


