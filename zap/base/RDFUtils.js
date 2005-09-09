/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.utils.importModule('resource:/jscodelib/zap/RDFUtils.js', null)" -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");

EXPORTED_SYMBOLS = [ "PersistentRDFObject"];

// name our global object:
function toString() { return "[RDFUtils.js]"; }

// object to hold module's documentation:
var _doc_ = {};

////////////////////////////////////////////////////////////////////////
// Globals

var gRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);


////////////////////////////////////////////////////////////////////////
// PersistentRDFObject

var PersistentRDFObject = makeClass("PersistentRDFObject", ErrorReporter);

// Hash of datasources where assertions for this resource will live.
// This will typically be bound by a subclass. Most persistent
// objects should have at least the default datasource 'default'.
PersistentRDFObject.obj("datasources", {});

// make sure each instance gets its own set of databases:
PersistentRDFObject.appendCtor(
  function() {
    this.datasources = objclone(this.datasources);
  });

// the actual resource:
PersistentRDFObject.obj("resource", null);

// array to hold descriptions of the resource's outgoing arcs
PersistentRDFObject.obj("_arcsOut", []);

// array to hold descriptions of the resource's incoming arcs
PersistentRDFObject.obj("_arcsIn", []);

// add an in-memory ephemeral datasource:
PersistentRDFObject.metafun(
  function addInMemoryDS(dsid) {
    this.appendCtor(
      function() {
        this.datasources[dsid] = Components.classes["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].createInstance(Components.interfaces.nsIRDFDataSource);
      });
  });

// add a literal attribute:
PersistentRDFObject.metafun(
  function rdfLiteralAttrib(_name, _defval, _datasourceid) {
    if (!_datasourceid) _datasourceid = "default";
    this.prototype._arcsOut.push({name:_name, defval:_defval,
                                  type:"literal", dsid:_datasourceid});
    this.gettersetter(
      _name,
      function() {
        var prop = gRDF.GetResource(_name);
        var target = this.datasources[_datasourceid].GetTarget(this.resource,
                                                               prop, true);
        if (target)
          target = target.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
        else
          target = _defval;
        return target;
      },
      function(val) {
        var prop = gRDF.GetResource(_name);
        var old = this.datasources[_datasourceid].GetTarget(this.resource, prop, true);
        if (old) {
          if (old.Value != val)
            this.datasources[_datasourceid].Change(this.resource, prop, old,
                                                   gRDF.GetLiteral(val), true);
        }
        else 
          this.datasources[_datasourceid].Assert(this.resource, prop,
                                                 gRDF.GetLiteral(val), true);
      });
  });

// add a resource attribute:
PersistentRDFObject.metafun(
  function rdfResourceAttrib(_name, _defval, _datasourceid) {
    if (!_datasourceid) _datasourceid = "default";
    this.prototype._arcsOut.push({name:_name, defval:_defval,
                                  type:"resource",dsid:_datasourceid});
    this.gettersetter(
      _name,
      function() {
        var prop = gRDF.GetResource(_name);
        var target = this.datasources[_datasourceid].GetTarget(this.resource,
                                                               prop, true);
        if (target)
          target = target.QueryInterface(Components.interfaces.nsIRDFResource).Value;
        else
          target = _defval;
        return target;
      },
      function(val) {
        var prop = gRDF.GetResource(_name);
        var old = this.datasources[_datasourceid].GetTarget(this.resource, prop, true);
        if (old) {
          if (old.Value != val)
            this.datasources[_datasourceid].Change(this.resource, prop, old,
                                                   gRDF.GetResource(val), true);
        }
        else 
          this.datasources[_datasourceid].Assert(this.resource, prop,
                                                 gRDF.GetResource(val), true);
      });
  });

// Add a triple(subject, predicate, this.resource) in datasource dsid
// when this object is initialized or created. Unassert in remove()
//
// Provide a workaround for a template builder limitation. The
// template builder always operates off some notion of containment
// with a 'container' variable and a 'member' variable. This means
// that it can't be used to generate content for a resource if there
// is no container ('ref') for the resources. 'rdfPointerAttrib'
// ensures that 'subject' is a suitable container. Usually for this
// usage, 'dsid' would be an ephemeral datasource.
PersistentRDFObject.metafun(
  function rdfPointerAttrib(_subject, _predicate, _dsid) {
    this.prototype._arcsIn.push({subject: _subject,
                                 predicate: _predicate,
                                 dsid:_dsid});
  });

// helper to assert all arcs in
PersistentRDFObject.fun(
  function _assertArcsIn() {
    var me = this;
    this._arcsIn.forEach(
      function(a) {
        var subj = gRDF.GetResource(a.subject);
        var pred = gRDF.GetResource(a.predicate);
        var target = me.datasources[a.dsid].GetTarget(subj, pred, true);
        if (target)
          me.datasources[a.dsid].Change(subj, pred, target, me.resource, true);
        else
          me.datasources[a.dsid].Assert(subj, pred, me.resource, true);
      });
  });

// init the resource with the given datasource & resource name:
PersistentRDFObject.fun(
  function initWithResource(resource) {
    this._assert(!this.resource, "already initialized");
    this.resource = resource;
    this._assertArcsIn();
  });

// create a new resource.
// if addAssertions==true, add default assertions for all attributes 
PersistentRDFObject.fun(
  function createNew(addAssertions) {
    this._assert(!this.resource, "already initialized");
    this.resource = gRDF.GetAnonymousResource();
    if (!addAssertions) return;
    this._assertArcsIn();
    // add default assertions for all attributes:
    var me = this;
    this._arcsOut.forEach(
      function(a) {
        var prop = gRDF.GetResource(a.name);
        var val;
        if (a.type == "literal")
          val = gRDF.GetLiteral(a.defval);
//         else if (a.type == "this")
//           val = me.resource;
        else
          val = gRDF.GetResource(a.defval);
        me.datasources[a.dsid].Assert(me.resource, prop, val, true);
      });
  });

// create a new resource from the given document:
PersistentRDFObject.fun(
  function createFromDocument(doc) {
    this._assert(!this.resource, "already initialized");
    this.resource = gRDF.GetAnonymousResource();
    this._assertArcsIn();
    // add assertions for all attributes:
    var me = this;
    this._arcsOut.forEach(
      function(a) {
        var prop = gRDF.GetResource(a.name);
        var val;
        var elem = doc.getElementById(a.name);
        if (!elem)
          val = a.defval;
        else if (elem.tagName == "checkbox")
          val = elem.checked;
        else
          val = elem.value;
        
        if (a.type == "literal")
          val = gRDF.GetLiteral(val);
//         else if (a.type == "this")
//           val = me.resource;
        else
          val = gRDF.GetResource(val);
        
        me.datasources[a.dsid].Assert(me.resource, prop, val, true);
      });
  });

// fill the given document with values from this resource, or with
// default values if there is no resource attached:
PersistentRDFObject.fun(
  function fillDocument(doc) {
    var me = this;
    if (this.resource) {
      this._arcsOut.forEach(
        function(a) {
          var elem = doc.getElementById(a.name);
          if (!elem) return;
          if (elem.tagName == "checkbox")
            elem.checked = (me[a.name] == "true");
          else
            elem.value = me[a.name];
        });
    }
    else {
      // fill with defaults:
      this._arcsOut.forEach(
        function(a) {
          var elem = doc.getElementById(a.name);
          if (!elem) return;
          if (elem.tagName == "checkbox") {
            elem.checked = a.defval;
          }
          else
            elem.value = a.defval;
        });
    }
  });

// update the resource from the given document:
PersistentRDFObject.fun(
  function updateFromDocument(doc) {
    this._assert(this.resource, "need resource to update!");
    
    // add/update assertions for all attributes:
    var me = this;
    this._arcsOut.forEach(
      function(a) {
        var prop = gRDF.GetResource(a.name);
        var val;
        var elem = doc.getElementById(a.name);
        if (!elem)
          val = a.defval;
        else if (elem.tagName == "checkbox")
          val = elem.checked;
        else
          val = elem.value;
        
        if (a.type == "literal")
          val = gRDF.GetLiteral(val);
//         else if (a.type == "this")
//           val = me.resource;
        else
          val = gRDF.GetResource(val);
        
        var old = me.datasources[a.dsid].GetTarget(me.resource, prop, true);
        if (old) {
          if (old.Value != val.Value)
            me.datasources[a.dsid].Change(me.resource, prop, old, val, true);
        }
        else
          me.datasources[a.dsid].Assert(me.resource, prop, val, true);
      });
  });


// remove this resource from the datasources, deleting all in- and out-arcs
PersistentRDFObject.fun(
  function remove() {
    for (var ds in this.datasources) {

      var datasource = this.datasources[ds];
      
      var arcsIn = datasource.ArcLabelsIn(this.resource);
      while (arcsIn.hasMoreElements()) {
        var arc = arcsIn.getNext();
        var sources = datasource.GetSources(arc, this.resource, true);
        while(sources.hasMoreElements()) {
          var source = sources.getNext();
          datasource.Unassert(source, arc, this.resource, true);
        }
      }
      
      var arcsOut = datasource.ArcLabelsOut(this.resource);
      while (arcsOut.hasMoreElements()) {
        var arc = arcsOut.getNext();
        var targets = datasource.GetTargets(this.resource, arc, true);
        while(targets.hasMoreElements()) {
          var target = targets.getNext();
          datasource.Unassert(this.resource, arc, target, true);
        }
      }
    }
    
    this.resource = null;
  });

PersistentRDFObject.fun(
  function flush() {
    for (var ds in this.datasources) {
      try {
        var remote = this.datasources[ds].QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
        remote.Flush();
      } catch(e) {
        // not a remote datasource
      }
    }
  });
