/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

const nsIRegion = Components.interfaces.nsIRegion;

function newRegion()
{
 var region = Components.classes["@mozilla.org/gfx/region;2"].createInstance(nsIRegion);
 return region;
}

r1 = newRegion();
r1.setToRect(0, 0, 400, 400);
dump("r1.isEmpty() == " + r1.isEmpty() + "\n");


r2 = newRegion();
dump("r2.isEmpty() == " + r2.isEmpty() + "\n");

