/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/jscodelib;1'].getService(Components.interfaces.mozIJSCodeLib).probeModule('resource:/jscodelib/xtf.js',1)" -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla XTF project.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

/**
 * Utilities for writing XTF elements.
 */

importModule("resource:/jscodelib/TemplateLib.js");

MOZ_EXPORTED_SYMBOLS = [ "ContentBuilder" ];

//----------------------------------------------------------------------
//ContentBuilder: simple interface to facilitate constructing xml
//content

var ContentBuilder = makeTemplate("ContentBuilder");

ContentBuilder.mergeTemplate(ErrorTemplate);

ContentBuilder.appendInitializer("ContentBuilder",
  function(args) {
    if (!args || !args.document) this.error("initializer arg 'document' missing");
    this._document = args.document;
    this._top = null;
    this._current = null;
    this._ns = null;
  });

ContentBuilder.addPrototypeObject("setElementNamespace",
  function(ns) {
    this._ns = ns;
    return this;
  });

ContentBuilder.addPrototypeObject("beginElement",
  function(tagname) {
    var elem = this._ns ? this._document.createElementNS(_ns, tagname) :
                          this._document.createElement(tagname);
    if (!this._current) {
      if (this._top) this.error("Building of multi-rooted trees not supported");
      this._current = this._top = elem;
    }
    else {
      this._current.appendChild(elem);
      this._current = elem;
    }
    return this;
  });

ContentBuilder.addPrototypeObject("endElement",
  function() {
    if (!this._current) this.error("Unbalanced begin/endElement");
    this._current = this._current.parentNode;
    return this;
  });

ContentBuilder.addPrototypeObject("attrib",
  function(name, value) {
    if (!this._current) this.error("No element");
    this._current.setAttribute(name, value);
    return this;
  });

ContentBuilder.addPrototypeGetter("root",
  function() {
    return this._top;
  });
