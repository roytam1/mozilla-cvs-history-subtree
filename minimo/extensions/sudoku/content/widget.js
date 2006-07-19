// REQUIRED FUNCTIONS

function setBigIcon(div, widgetDir) {

	var wrapper = document.createElement("div");
	wrapper.style.backgroundColor = "white";
	wrapper.style.height="35px";
	wrapper.style.width="100%";
	wrapper.style.overflow = "hidden";
	wrapper.style.position = "absolute";
	wrapper.style.top = "50%";
	wrapper.style.marginTop = "-17px";


	var c = document.createElement("center");
	c.style.fontSize = "10px";
	c.innerHTML = '#<br/>SUDOKU<br/>#'

	wrapper.appendChild(c);
	div.appendChild(wrapper);
}

function setSmallIcon(div, widgetDir) {
	div.innerHTML = '<center>#</center>';
}

function getInterval() {
	return -1;
}

function receivedMedia() {
	return;
}

function getRightSoftkeyLabel() {
	return "Game Options";
}

function rightSoftkeyPressed() {
	alert("wrong rightSoftkey Pressed");
	return;
}