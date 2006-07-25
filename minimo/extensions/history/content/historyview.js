


function bmInit(targetDoc, targetElement) {

	var testLoad=new bmProcessor();
	testLoad.xslSet("history_template.xml");
	testLoad.setTargetDocument(targetDoc);
	testLoad.setTargetElement(targetElement);
	testLoad.run();

}

function bmProcessor() {



      /* init stuff */ 
  

	this.xmlRef=document.implementation.createDocument("","",null);
	this.xslRef=document.implementation.createDocument("http://www.w3.org/1999/XSL/Transform","stylesheet",null);

	var myObserver = null;


	try { 
	
	  var mySearch = Components.classes["@mozilla.org/autocomplete/search;1?name=history"].
	                            getService(Components.interfaces.nsIAutoCompleteSearch);
	
	  myObserver = {
	      onSearchResult:function (q,w) { 
	        rr=w.QueryInterface(Components.interfaces.nsIAutoCompleteResult);
		  this.bookmarkStore="<bm>";
	        for(var ii=0;ii<rr.matchCount;ii++) {
	          this.bookmarkStore+="<li title='"+rr.getValueAt(ii)+"'>"+rr.getValueAt(ii)+"</li>";
	        }
		  this.bookmarkStore+="</bm>";
	      },
            bookmarkStore:""
	  }; 
	
	mySearch.startSearch("","",null, myObserver );
	
	} catch (ignore) {

	}

	var aDOMParser = new DOMParser();
	this.xmlRef = aDOMParser.parseFromString(myObserver.bookmarkStore,"text/xml");

	this.xslUrl="";

	var myThis=this;
	var omega=function thisScopeFunction2() { myThis.xslLoaded(); }

	this.xslRef.addEventListener("load",omega,false);

	this.xmlLoadedState=true;
	this.xslLoadedState=false;


}

bmProcessor.prototype.xmlLoaded = function () {
	this.xmlLoadedState=true;	
alert('loaded');
	this.apply();
}

bmProcessor.prototype.xslLoaded = function () {
	this.xslLoadedState=true;
	this.apply();
}

bmProcessor.prototype.xmlSet = function (urlstr) {
	this.xmlUrl=urlstr;
}

bmProcessor.prototype.xslSet = function (urlstr) {
	this.xslUrl=urlstr;
}

bmProcessor.prototype.setTargetDocument = function (targetDoc) {
	this.targetDocument=targetDoc;
}

bmProcessor.prototype.setTargetElement = function (targetEle) {
	this.targetElement=targetEle;
}

bmProcessor.prototype.apply = function () {
    if( this.xmlRef.getElementsByTagName("li").length < 1) {
      if( this.targetDocument && this.targetDocument ) {
        if(this.targetDocument.getElementById("message-empty")) {

            this.targetDocument.getElementById("message-empty").style.display="block";
        }
        // ... other checks? other formatting...
      } 
      return; 
    }

    if(this.xmlLoadedState&&this.xslLoadedState) {	
        var xsltProcessor = new XSLTProcessor();
        var htmlFragment=null;
        try {
          xsltProcessor.importStylesheet(this.xslRef);
          htmlFragment = xsltProcessor.transformToFragment(this.xmlRef, this.targetDocument);
        } catch (e) {
        }
        this.targetElement.appendChild(htmlFragment.firstChild);
    }    
}

bmProcessor.prototype.run = function () {
	try {
		// Already parsed.
		// this.xmlRef.load(this.xmlUrl);
	} catch (e) {
	}
	try {
		this.xslRef.load(this.xslUrl);
	} catch (e) {
	}

}

