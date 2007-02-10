var CUR_I = 0;
var CUR_J = 0;
var HIGHLIGHTED_BOX_COLOR = "#FF6600";
var UNEDITABLE_BOX_COLOR = "#555555";
var ERROR_GROUP_COLOR = "#333333";
var ERROR_BOX_COLOR = "white";

var g_usrGrid = null;
var g_clear_row = null;
var g_clear_col = null;
var g_clear_box = null; // 2d array, i offset / j offset


document.addEventListener("keydown", keyDown, false);

function keyDown(e) {
	var kc = e.keyCode;
	var detect = navigator.userAgent.toLowerCase();
		if(detect.match("opera")){
			switch (kc) {
				case 13:
				case 37:
				case 38:
				case 39:
				case 40:
					return;
			}
	}
	keyHelp(kc);
}

function keyHelp(kc) {

	var i = null;
	var j = null;
	var editable = null;

	if (g_clear_row || g_clear_row == 0) {
		document.getElementById("container"+g_clear_row+"_0").parentNode.parentNode.bgColor = "";
		for (var z=0;z<9;z++) {
			var td = document.getElementById("container"+g_clear_row+"_"+z).parentNode;
			td.bgColor = "";
		}
		g_clear_row = null;
	}

	if (g_clear_col || g_clear_col == 0) {
		for (var count = 0; count<9; count++) {
			var td = document.getElementById("container"+count+"_"+g_clear_col).parentNode;
			td.bgColor ="";
		}
		g_clear_col = null;
	}

	if (g_clear_box) {
		for (var x=0;x<9; x++) {
			var i_box = g_clear_box[0] + ((x - (x%3))/3);
			var j_box = g_clear_box[1] + ( x%3 );
			var td = document.getElementById("container"+i_box+"_"+j_box).parentNode;
			td.bgColor = "";
		}
		g_clear_box = null;
	}

	var myContainer = document.getElementById("container" + CUR_I+"_"+CUR_J);
	editable = myContainer.getAttribute("editable") == "true";
	switch (kc) {
		//left
		case 37:{
			j = CUR_J - 1;
			i = CUR_I;
			if (j < 0 && i!=0) {
				j = 8;
				i--;
			} else {
				if (j < 0) {
					j = 0;
					i = 0;
				}
			}
		}
		break;

		//up
		case 38: {
			j = CUR_J;
			i = CUR_I - 1;
			if (i < 0)
				i = 0;
		}
		break;

		//right
		case 39:{
			j = CUR_J + 1;
			i = CUR_I;
			if (j > 8) {
				j = 0;
				i++;
			}
			if (i > 8) {
				i = 8;
			}
		}
		break;

		//down
		case 40:{
			j = CUR_J;
			i = CUR_I + 1;
			if(i > 8){
				i = 8;
				j = 8;
			}
		}
		break;

		// numbers 1-9 (keyCodes 49-57)
		case 49:
		if (g_startGame) {
			buttonAction('easy');
			//rightSoftkeyPressed();
			break;
		}
		case 50:
		if (g_startGame) {
			buttonAction('medium');
			//rightSoftkeyPressed();
			break;
		}
		case 51:
		if (g_startGame) {
			buttonAction('hard');
			//rightSoftkeyPressed();
			break;
		}
		case 52:
		case 53:
		case 54:
		case 55:
		case 56:
		case 57:
		if(editable){
			var x = new sudokuNumber();
			var myNum = (kc - 48);
			var ret = x.check(myNum, CUR_I, CUR_J, g_usrGrid.data);
			var usr_div = document.getElementById("user"+CUR_I+"_"+CUR_J);
			if(ret) {
				usr_div.innerHTML = myNum;
				var finished = true;
				for (var zz=0; zz<g_usrGrid.data.length; zz++) {
					if (finished == false)
						break;
					for (var zzz=0;zzz<g_usrGrid.data[zz].length; zzz++) {
						if (g_usrGrid.data[zz][zzz] == null) {
							finished = false;
							break;
						}
					}
				}
				if (finished) {
					alert("GOOD JOB!\nYOU WIN!!");
				}
			} else {
				switch (x.errorType) {
					case "row":
						// user -> container -> table cell -> table row
						var tr = usr_div.parentNode.parentNode.parentNode;
						tr.bgColor = ERROR_GROUP_COLOR;
						var cells = tr.cells;
						for (var count = 0; count<cells.length; count++) {
							var node = cells.item(count);
							var container = document.getElementById("container"+CUR_I+"_"+count);
							var valToCompare = null;
							if (container.getAttribute("editable") == "true")
								valToCompare = document.getElementById("user"+CUR_I+"_"+count);
							else
								valToCompare = document.getElementById("box"+CUR_I+"_"+count);
							if (parseInt(valToCompare.innerHTML) == myNum)
								node.bgColor = ERROR_BOX_COLOR;
						}
						g_clear_row = CUR_I;
						break;
					case "col":
						for (var count=0; count<9; count++){
							var container = document.getElementById("container"+count+"_"+CUR_J);
							var td = container.parentNode;
							var valToCompare = null;
							if (container.getAttribute("editable") == "true")
								valToCompare = document.getElementById("user"+count+"_"+CUR_J);
							else
								valToCompare = document.getElementById("box"+count+"_"+CUR_J);

							if (parseInt(valToCompare.innerHTML) == myNum)
								td.bgColor = ERROR_BOX_COLOR;
							else
								td.bgColor = ERROR_GROUP_COLOR;
						}
						g_clear_col = CUR_J;
						break;
					case "box":
						var i_off = (CUR_I-( CUR_I%3));
						var j_off = (CUR_J-( CUR_J%3));
						for(x = 0; x<9; x++){
							var i_box = i_off+ ((x - (x%3))/3);
							var j_box = j_off+ ( x%3 );
							var container = document.getElementById("container"+i_box+"_"+j_box);
							var td = container.parentNode;
							var valToCompare = null;
							if (container.getAttribute("editable") == "true")
								valToCompare = document.getElementById("user"+i_box+"_"+j_box);
							else
								valToCompare = document.getElementById("box"+i_box+"_"+j_box);
							if (parseInt(valToCompare.innerHTML) == myNum)
								td.bgColor = ERROR_BOX_COLOR;
							else
								td.bgColor = ERROR_GROUP_COLOR;
						}
						g_clear_box = new Array();
						g_clear_box[0] = i_off;
						g_clear_box[1] = j_off;
						break;
				}
				usr_div.innerHTML = "";
			}
		} else{
			alert("You cannot modify this box.");
		}
		break;

		case 48:
		if (g_startGame) {
			toggleAnswers();
			rightSoftkeyPressed();
			break;
		}

		// backspace and delete on a mac
		case 8:
		case 46:
		if (editable) {
			g_usrGrid.data[CUR_I][CUR_J] = null;
			var usr_div = document.getElementById("user"+CUR_I+"_"+CUR_J);
			usr_div.innerHTML = "";
		}
		break;
	}

	if (i!=null && j!=null) {
		var oldBox = document.getElementById("container"+CUR_I+"_"+CUR_J);
		oldBox.style.backgroundColor = "transparent";

		var newBox = document.getElementById("container"+i+"_"+j);
		newBox.style.backgroundColor = HIGHLIGHTED_BOX_COLOR;
		CUR_I = i;
		CUR_J = j;
	}
}

g_startGame = false;
function rightSoftkeyPressed() {
	var body = document.getElementsByTagName("body")[0];
	if (!document.getElementById("greyOut")) {
		g_startGame = true;
		var greyOut = document.createElement("div");
		greyOut.style.backgroundImage = "url(\"grey-out.png\")";
		greyOut.style.backgroundRepeat = "repeat";
		greyOut.style.width = "100%";
		greyOut.style.height= "100%";
		greyOut.style.position = "absolute";
		greyOut.style.top = "0px";
		greyOut.style.left = "0px";
		greyOut.id = "greyOut";
		greyOut.style.zIndex = "3";
		body.appendChild(greyOut);

		var dif = document.createElement("div");
		dif.style.backgroundColor = HIGHLIGHTED_BOX_COLOR;
		dif.style.height="150px";
		dif.style.width="150px";
		dif.style.position = "absolute";
		dif.style.top = "50%";
		dif.style.marginTop = "-75px";
		dif.style.left = "50%";
		dif.style.marginLeft = "-75px";
		dif.style.zIndex = "5";
		dif.id = "dif";
		
	
		var wrapper = document.createElement("div");
		wrapper.style.height="80px";
		wrapper.style.width="150px";
		wrapper.style.position = "absolute";
		wrapper.style.top = "50%";
		wrapper.style.marginTop = "-40px";
		wrapper.style.zIndex = "6";
		dif.appendChild(wrapper);

		var wait = document.createElement("img");
		wait.style.position = "absolute";
		wait.style.top = 25;
		//wait.style.marginTop = "-50%";
		wait.style.left = 60;
		//wait.style.marginLeft = "-50%";
		wait.style.textAlign = "center";
		wait.style.zIndex = "7";
		wait.src="wait.gif";
		wait.id = "wait";
		wait.style.display="none";
		wrapper.appendChild(wait);
		


		var c = document.createElement("center");
		c.style.fontSize = "10px";
		c.style.fontFamily = "Arial";
		c.innerHTML = "<b>Start New Game</b><br/>1 - Easy<br/>2 - Medium<br/>3 - Hard<br/><br/>0 - Toggle Answers";
		wrapper.appendChild(c);
		body.appendChild(dif);

		setSoftkeyLabel("Cancel");
	} else {
		g_startGame = false;
		body.removeChild(document.getElementById("greyOut"));
		body.removeChild(document.getElementById("dif"));
		setSoftkeyLabel("Game Options");
	}
}

function myDump(input){
	dump(input + "\n");
}

function boardWrite(i,j, input) {
	document.getElementById("box" + i + "_" + j).innerHTML = input;
}

function init() {
	var body = document.getElementsByTagName("body")[0];
	var table = document.createElement("table");
	table.id = "gameBoard";
	table.style.width = "175px";
	table.style.height = "175px";
	table.border=1;
	table.cellPadding=0;
	table.cellSpacing=0;
	document.getElementById("myBoard").appendChild(table);

	for(var i =0; i<9; i++){
		var row = document.createElement("tr");
		row.align = "center";
		table.appendChild(row);
		for( var j = 0; j<9; j++){
		    var col = document.createElement("td");
		    row.appendChild(col);

		    var container = document.createElement("div");
		    container.id = "container"+i+"_"+j;
		    container.style.height= "14px";
		    container.style.width = "14px";
		    container.style.overflow = "hidden";
		    container.style.fontSize = "9px";

		    var div = document.createElement("div");
		    div.id="box"+i+"_"+j;
		    div.style.display = "none";
		    div.style.height ="14px";
		    div.style.width = "14px";
		    div.style.top = "0px";
		    div.style.fontSize = "9px";

		    var usr_div = document.createElement("div");
		    //usr_div.innerHTML = "#";
		    usr_div.id="user"+i+"_"+j;
		    usr_div.style.display = "block";
		    usr_div.style.height ="14px";
		    usr_div.style.width = "14px";
		    usr_div.style.top = "0px";
		    usr_div.style.fontSize = "9px";
		    usr_div.style.fontWeight = "bold";

		    container.appendChild(div);
		    container.appendChild(usr_div);
		    col.appendChild(container);
		    container.addEventListener("click", divClicked, false);
		}
	}
}

function buttonAction(input) {
    var wait = document.getElementById("wait");
    wait.style.display = "block";
    setTimeout(buttonActionHelper,1000, input,wait);
}
function clearTable(input,wait){
    for (var i=0; i < 9; i++){
	setTimeout(clearRow,1,i,input,wait);
	}
}
function clearRow(i,input,wait){
    for (var j=0; j<9; j++) {
	setTimeout(clearCell,1,i,j,input,wait);
    }
}
function clearCell(i,j,input,wait){
    			var myBox = document.getElementById("box"+i+"_"+j);
			var myContainer = document.getElementById("container"+i+"_"+j);
			var usrBox = document.getElementById("user"+i+"_"+j);
			usrBox.innerHTML = "";
			myBox.innerHTML = "";
			myContainer.setAttribute("editable", true);
			myContainer.style.backgroundColor = "transparent";
			myBox.style.display = "none";
			myBox.style.color = "black";
			if(i ==8 && j ==8){
			    setTimeout(buttonActionHelper2,1, input,wait);
			}
}
function buttonActionHelper(input, wait){
	document.getElementById("gameBoard").border = "1";
	setTimeout(clearTable,1,input,wait);
	
}
function buttonActionHelper2(input, wait){

	if (b_answers){
		b_answers = !b_answers;
	}

	var myGrid = new sudokuGrid();
	g_usrGrid = new sudokuGrid();
	myGrid.fillGrid(input);
    setTimeout(buttonActionHelper3,1, input,wait);
}
function buttonActionHelper3(input, wait){

	CUR_I = 0;
	CUR_J = 0;
	var startBox = document.getElementById("container"+CUR_I+"_"+CUR_J);
	startBox.style.backgroundColor = HIGHLIGHTED_BOX_COLOR;
     
	wait.style.display="none";
	rightSoftkeyPressed();
}

var b_answers = false;
function toggleAnswers() {
	b_answers = !b_answers;
	for (var i=0; i<9; i++) {
		for (var j=0;j<9;j++) {
			var box = document.getElementById("box"+i+"_"+j);
			if (b_answers){
				if (box.style.display == "block")
					box.style.display = "inline";
				else
					box.style.display = "block";
			} else {
				if (box.style.display == "block")
					box.style.display = "none";
				else
					box.style.display = "block";
			}
		}
	}
}

function divClicked() {
	var oldBox = document.getElementById("container"+CUR_I+"_"+CUR_J);
	oldBox.style.backgroundColor = "transparent";
	var newBox = this;
	newBox.style.backgroundColor = HIGHLIGHTED_BOX_COLOR;

	var coordinates = this.id.substr(9);
	var coordSplit = coordinates.indexOf("_");
	CUR_I = parseInt(coordinates.substr(0,coordSplit));
	CUR_J = parseInt(coordinates.substr(coordSplit+1));
}

var sudokuGrid = function() {
	this.data = new Array();

	// create a block for all the rows
	for (var i = 0; i < 9; i++) {
		this.data[i] = new Array();
	}

	this.fillGrid = function(difficulty) {
		var map = new Array(1,2,3,4,5,6,7,8,9);
		for (var m = 0; m <9; m++){
			var tmp = map[m];
			var ti =  Math.floor(Math.random()*9);
			map[m] = map[ti];
			map[ti] = tmp;
		}
		for (var i = 0; i< 9; i++) {
			for( var j=0; j<9; j++) {
				var v =  (((i*3)+j) +((i-( i%3))/3  )  )%9;
				this.setBox(i,j,map[v]);
			}
		}
		this.doSwapping();
		this.refreshBoard();

		for(var i=0; i < 9; i++) {
			for (var j=0; j<9; j++) {
				var x = Math.random();
				var myBox = document.getElementById("box"+i+"_"+j);
				var myContainer = document.getElementById("container"+i+"_"+j);
				var usrBox = document.getElementById("user"+i+"_"+j);
				switch(difficulty) {
					case 'easy':
					if (x < .5){
						myBox.style.display = "block";
						myBox.style.color = UNEDITABLE_BOX_COLOR;
						myContainer.setAttribute("editable", false);
						var z = new sudokuNumber();
						z.check(parseInt(myBox.innerHTML),i,j,g_usrGrid.data);
					}
					break;
					case 'medium':
					if (x < .3333333){
						myBox.style.display = "block";
						myBox.style.color = UNEDITABLE_BOX_COLOR;
						myContainer.setAttribute("editable", false);
						var z = new sudokuNumber();
						z.check(parseInt(myBox.innerHTML),i,j,g_usrGrid.data);
					}
					break;
					case 'hard':
					if (x < .2){
						myBox.style.display = "block";
						myBox.style.color = UNEDITABLE_BOX_COLOR;
						myContainer.setAttribute("editable", false);
						var z = new sudokuNumber();
						z.check(parseInt(myBox.innerHTML),i,j,g_usrGrid.data);
					}
					break;
				}
			}
		}

	}

	this.doSwapping = function(){
	    for( var i = 0; i<1; i++){
		this.swapBlockRows();
		this.swapBlockCols();
	    }
	    for(var j = 0; j<1; j++){
		for(var i = 0; i<3; i++){
		    this.swapRows(i);
		    this.swapCols(i);
		}
	    }
	}

	this.swapRows = function(x){
	     var row1 = Math.floor(Math.random()*3);
	     var row2 = Math.floor(Math.random()*3);
	     this.swapRow(x*3+row1, x*3+row2);
	}
	this.swapCols = function(x){
	    var col1 = Math.floor(Math.random()*3);
	    var col2 = Math.floor(Math.random()*3);
	    this.swapCol(x*3+col1, x*3+col2);
	}

	this.swapBlockRows = function(){
	    var row1 = Math.floor(Math.random()*3);
	    var row2 = Math.floor(Math.random()*3);

	    this.swapBlockRow(row1,row2);
	}
	this.swapBlockRow = function(row1,row2){
	    for(var i = 0 ;i< 3 ;i++){
		this.swapRow(i+(3*row1), i+(3*row2));
	    }
	}
	this.swapBlockCols = function(){
	    var col1 = Math.floor(Math.random()*3);
	    var col2 = Math.floor(Math.random()*3);

	    this.swapBlockCol(col1,col2);
	}

	this.swapBlockCol = function(col1,col2){
	   for(var i = 0 ;i< 3 ;i++){
	       this.swapCol(i+(3*col1), i+(3*col2));
	   }
	}

	this.swapCol = function(i1,i2){
	    for(var j =0; j<9; j++){
		    var tmp = this.getBox(i1,j);
		    this.setBox(i1,j, this.getBox(i2,j));
		    this.setBox(i2,j, tmp);
	     }
	}
	this.swapRow = function(j1,j2){
	    for(var i =0; i<9; i++){
		    var tmp = this.getBox(i,j1);
		    this.setBox(i,j1, this.getBox(i,j2));
		    this.setBox(i,j2, tmp);
	     }
	}

	this.getBox = function(i, j) {
		return (this.data[i][j]);
	}

	this.setBox = function(i, j, data) {
		this.data[i][j] = data;
	}

	this.refreshBoard = function() {
		for (var i = 0; i< 9; i++) {
			for( var j=0; j<9; j++) {
				boardWrite(i,j,this.getBox(i,j));
			}
		}
	}

}

var sudokuNumber = function() {
	this.history = new Array(0,0,0,0,0,0,0,0,0);
	this.val = 0;
	this.errorType = null;

	this.check = function(input, row, column, data) {
		data[row][column] = input;

		// have we already checked this number?
		if (this.history[input-1] == 1){
			//myDump("history problem for " + input);
			return false;
		}

		// check off this number from being checked again
		this.history[input-1] = 1;
		this.val = input;

		var ret = this.checkRow(row,column,data);
		if (!ret){
			this.errorType = "row";
			data[row][column] = null;
			return false;
		}
		ret = this.checkCol(row,column,data);
		if (!ret){
			this.errorType = "col";
			data[row][column] = null;
			return false;
		}
		ret = this.checkBox(row,column,data);
		if (!ret){
			this.errorType = "box";
			data[row][column] = null;
			return ret;
		}
		this.errorType = null;
		return ret;
	}

this.checkCol = function (i,j,data){
	// hold j the same and check all rows
    var tmp = Array();
    for(i = 0; i<9; i++){
	if(!data[i] || !data[i][j] || data[i][j]==""){
	}else if(((data[i][j] >0 && data[i][j]<10)) && tmp[data[i][j]]!='y'){

	    tmp[data[i][j]]='y';
	}else{
	    //myDump("row problem for " + i+","+j);
	    return false;
	}
    }
    return true;
}
this.checkRow = function (i,j,data){
	// hold i the same and check all the columns
    var tmp = Array();
    for(j = 0; j<9; j++){
	if(!data[i] || !data[i][j] || data[i][j]==""){
	}else if(data[i][j] >0 && data[i][j]<10 && tmp[data[i][j]]!='y'){

	    tmp[data[i][j]]='y';
	}else{
		//myDump("col problem for " +i+","+j);
	    return false;
	}
    }
    return true;
}

	this.checkBox = function (i_orig,j_orig,data){
		var i_off = (i_orig-( i_orig%3));
		var j_off = (j_orig-( j_orig%3));
		var tmp = Array();
		for(x = 0; x<9; x++){
			var i = i_off+ ((x - (x%3))/3);
			var j = j_off+ ( x%3 );

			if(!data[i]){
				// do nothing
			}else if(!data[i][j] || data[i][j] == ""){
				// do nothing
			}else if(data[i][j] >0 && data[i][j]<10 && tmp[data[i][j]]!='y'){
				//first time we've seen this number, record it
				tmp[data[i][j]]='y';
			}else{
				//myDump("box problem for " + i_orig +","+j_orig);
				return false;
			}
		}
		return true;
	}
}