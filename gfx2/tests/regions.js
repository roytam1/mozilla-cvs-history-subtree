/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

const nsIRegion = Components.interfaces.nsIRegion;

r1 = Components.classes["mozilla.gfx.region.2"].createInstance(nsIRegion)
r1.setToRect(0, 0, 400, 400);
dump("r1.isEmpty() == " + r1.isEmpty() + "\n");


r2 = Components.classes["mozilla.gfx.region.2"].createInstance(nsIRegion)
dump("r2.isEmpty() == " + r2.isEmpty() + "\n");

