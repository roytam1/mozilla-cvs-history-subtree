/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/RDFUtils.jsm', null)" -*- */
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

Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");

EXPORTED_SYMBOLS = [ "PersistentRDFObject"];

// name our global object:
// function toString() { return "[RDFUtils.jsm]"; }


////////////////////////////////////////////////////////////////////////
// Globals

var gRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);


////////////////////////////////////////////////////////////////////////
// PersistentRDFObject

var PersistentRDFObject = makeClass("PersistentRDFObject", ErrorReporter);

// initHook will be called when the object is constructed (from
// initWithResource, createNew, or createFromDocument):
PersistentRDFObject.obj("initHook", null);

// removeHook will be called when the object is removed from all
// datasources (from remove):
PersistentRDFObject.obj("removeHook", null);

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

// hash to hold descriptions of the resource's outgoing arcs
PersistentRDFObject.obj("_arcsOut", {});

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
    this.prototype._arcsOut[_name] = {name:_name, defval:_defval,
                                      type:"literal", dsid:_datasourceid,
                                      triggers:[]};
    this.gettersetter(
      _name,
      function() {
        var prop = gRDF.GetResource(_name);
        var target = null;
        try {
          if (this.resource) 
            target = this.datasources[_datasourceid].GetTarget(this.resource,
                                                               prop, true);
        }
        catch(e) {
//          this._error("Exception getting "+_name+": "+e);
        }
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
          if (old.QueryInterface(Components.interfaces.nsIRDFLiteral).Value == val) return;
          this.datasources[_datasourceid].Change(this.resource, prop, old,
                                                 gRDF.GetLiteral(val), true);
        }
        else
          this.datasources[_datasourceid].Assert(this.resource, prop,
                                                 gRDF.GetLiteral(val), true);
        if (this.autoflush) {
          try {
            this.datasources[_datasourceid].QueryInterface(Components.interfaces.nsIRDFRemoteDataSource).Flush();
          } catch (e) { /* not a remote datasource */ }
        }

        this._executeTriggers(this._arcsOut[_name], gRDF.GetLiteral(val));
      });
  });

// Add a resource attribute:
PersistentRDFObject.metafun(
  function rdfResourceAttrib(_name, _defval, _datasourceid) {
    if (!_datasourceid) _datasourceid = "default";
    this.prototype._arcsOut[_name] = {name:_name, defval:_defval,
                                      type:"resource",dsid:_datasourceid};
    this.gettersetter(
      _name,
      function() {
        var prop = gRDF.GetResource(_name);
        var target = null;
        try {
          if (this.resource) 
            target = this.datasources[_datasourceid].GetTarget(this.resource,
                                                               prop, true);
        }
        catch(e) {}
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
        if (this.autoflush) {
          try {
            this.datasources[_datasourceid].QueryInterface(Components.interfaces.nsIRDFRemoteDataSource).Flush();
          } catch (e) { /* not a remote datasource */ }
        }
      });
  });

// Add a trigger that will be executed whenever the given attribute is
// initialized or changed by a setter call on this PersistentRDFObject.
// The trigger function takes 2 arguments: the name of the observed
// resource and the new value of the observed attribute. It will be
// called with 'this' set to the PersistentRDFObject.
// XXX maybe need this to execute even if the attrib gets manipulated
// from elsewhere.
PersistentRDFObject.metafun(
  function rdfAttribTrigger(_name, _function) {
    this._assert(this.prototype._arcsOut[_name] &&
                 this.prototype._arcsOut[_name].triggers,
                 "Trying to set trigger for unknown or incompatible property "+_name);
    // XXX cloning triggers here is a little bit of a hack so that we
    // don't pollute the base class's triggers array: Merging doesn't
    // do deep cloning, so the triggers arrays of base- and subclasses
    // are the same object. What we really want is deep cloning on
    // merging (at least for this particular case)
    var arcObj = objclone(this.prototype._arcsOut[_name]);
    arcObj.triggers = arrayclone(arcObj.triggers);
    arcObj.triggers.push(_function);
    this.prototype._arcsOut[_name] = arcObj;
  });

// helper to execute all triggers for the given attribute:
PersistentRDFObject.fun(
  function _executeTriggers(attrib, val) {
    if (!attrib.triggers || !attrib.triggers.length) return;
    var me = this;
    var value;
    // we need to QI here, since e.g. vals returned by GetTarget will be
    // just nsIRDFNodes...
    if (attrib.type == "literal")
      value = val.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
    else
      value = val.QueryInterface(Components.interfaces.nsIRDFResource).Value;

    var args = [attrib.name, value];
    this._dump("calling triggers for "+args[0]+", value="+args[1]);
    attrib.triggers.forEach(function(f) { f.apply(me, args); });
  });

// Add a triple(subject, predicate, this.resource) in datasource dsid
// when this object is initialized or created. Unassert in remove().
// Existing arcs with the same subject and predicate will not be changed.
// A new arcs will be created alongside them.
//
// Provide (among other things) a workaround for a template builder
// limitation. The template builder always operates off some notion of
// containment with a 'container' variable and a 'member'
// variable. This means that it can't be used to generate content for
// a resource if there is no container ('ref') for the
// resources. 'rdfPointerAttrib' ensures that 'subject' is a suitable
// container. Usually for this usage, 'dsid' would be an ephemeral
// datasource.
PersistentRDFObject.metafun(
  function rdfPointerAttrib(_subject, _predicate, _dsid) {
    this.prototype._arcsIn.push({subject: _subject,
                                 predicate: _predicate,
                                 dsid:_dsid});
  });

PersistentRDFObject.fun(
  function addAssertion(name, val, dsid) {
    this.datasources[dsid].Assert(this.resource,
                                  gRDF.GetResource(name),
                                  val,
                                  true);
  });

PersistentRDFObject.fun(
  function removeAssertion(name, val, dsid) {
    this.datasources[dsid].Unassert(this.resource,
                                    gRDF.GetResource(name),
                                    val,
                                    true);
  });


// helper to assert all arcs in
PersistentRDFObject.fun(
  function _assertArcsIn() {
    var me = this;
    this._arcsIn.forEach(
      function(a) {
        var subj = gRDF.GetResource(a.subject);
        var pred = gRDF.GetResource(a.predicate);
//         var target = me.datasources[a.dsid].GetTarget(subj, pred, true);
//         if (target)
//           me.datasources[a.dsid].Change(subj, pred, target, me.resource, true);
//         else
          me.datasources[a.dsid].Assert(subj, pred, me.resource, true);
        //XXX should we flush here?
      });
  });

// init the resource with the given datasource & resource name:
PersistentRDFObject.fun(
  function initWithResource(resource) {
    this._assert(!this.resource, "already initialized");
    this.resource = resource;
    this._assertArcsIn();

    // make sure we have assertions for all attributes:
    for (var id in this._arcsOut) {
      var a = this._arcsOut[id];
      var prop = gRDF.GetResource(a.name);
      var val = this.datasources[a.dsid].GetTarget(this.resource, prop, true);
      if ( val == null) {
        // add default assertion:
        if (a.type == "literal")
          val = gRDF.GetLiteral(a.defval);
//         else if (a.type == "this")
//           val = this.resource;
        else
          val = gRDF.GetResource(a.defval);
        
        this.datasources[a.dsid].Assert(this.resource, prop,
                                        val, true);
      }
      this._executeTriggers(a, val);
    }

    if (this.initHook)
      this.initHook();
    
    if (this.autoflush)
      this.flush();
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
    for (var id in this._arcsOut) {
      var a = this._arcsOut[id];
      var prop = gRDF.GetResource(a.name);
      var val;
      if (a.type == "literal")
        val = gRDF.GetLiteral(a.defval);
//    else if (a.type == "this")
//      val = this.resource;
      else
        val = gRDF.GetResource(a.defval);
      this.datasources[a.dsid].Assert(this.resource, prop, val, true);
      this._executeTriggers(a, val);
    }
    
    if (this.initHook)
      this.initHook();
    
    if (this.autoflush)
      this.flush();
  });

// create a new resource from the given document:
PersistentRDFObject.fun(
  function createFromDocument(doc) {
    this._assert(!this.resource, "already initialized");
    this.resource = gRDF.GetAnonymousResource();
    this._assertArcsIn();
    // add assertions for all attributes:
    for (var id in this._arcsOut) {
      var a = this._arcsOut[id];
      var prop = gRDF.GetResource(a.name);
      var elem = doc.getElementById(a.name);
      var val = elem ? elem.value : a.defval;
      
      if (a.type == "literal")
        val = gRDF.GetLiteral(val);
//    else if (a.type == "this")
//      val = this.resource;
      else
        val = gRDF.GetResource(val);
        
      this.datasources[a.dsid].Assert(this.resource, prop, val, true);
      this._executeTriggers(a, val);
    }

    if (this.initHook)
      this.initHook();

    if (this.autoflush)
      this.flush();
  });

// fill the given document with values from this resource, or with
// default values if there is no resource attached:
PersistentRDFObject.fun(
  function fillDocument(doc) {
    for (var id in this._arcsOut) {
      var a = this._arcsOut[id];
      var elem = doc.getElementById(a.name);
      if (!elem) continue;
      elem.value = this[a.name];
      if (elem.isZapFormWidget) {
        // reset modified & invalid state:
        // XXX regexp test???
        elem.state = 0x0000;
        elem.defaultVal = elem.value;
      }
    }
  });

// Initialize a XUL template with the datasources of this PersistentRDFObject.
// 'node' should be the parent DOM node of a <template> element.
// If 'removeOldDataSources' is true, old datasources will be removed from the
// template before adding the new datasources.
// After adding the PersistentRDFObject's datasources the template will be rebuilt.
PersistentRDFObject.fun(
  function initXULTemplate(node, removeOldDataSources) {
    if (removeOldDataSources) {
      var enumerator = node.database.GetDataSources();
      while (enumerator.hasMoreElements())
        node.database.RemoveDataSource(enumerator.getNext());
    }
    for (var ds in this.datasources) {
      node.database.AddDataSource(this.datasources[ds]);
    }
    node.builder.rebuild();
  });

// update the resource from the given document:
PersistentRDFObject.fun(
  function updateFromDocument(doc) {
    this._assert(this.resource, "need resource to update!");
    
    // add/update assertions for all attributes:
    for (var id in this._arcsOut) {
      var a = this._arcsOut[id];
      var prop = gRDF.GetResource(a.name);
      var val;
      var elem = doc.getElementById(a.name);
      if (!elem) {
        // check if the datasource already has an assertion with this
        // predicate:
        if (this.datasources[a.dsid].GetTarget(this.resource, prop, true) != null) {
          dump("Already have an assertion for "+a.name+". Not adding default assertion.\n");
          continue; // yes -> don't add a default assertion
        }
        dump("No assertion for "+a.name+". Adding default assertion ("+a.defval+").\n");
        // no -> proceed to add default assertion
        val = a.defval;
      }
      else {
        if (elem.isZapFormWidget) {
          // reset modified & invalid state:
          // XXX regexp test???
          elem.state = 0x0000;
          elem.defaultVal = elem.value;
        }      
        val = elem.value;
      }
      
      if (a.type == "literal")
        val = gRDF.GetLiteral(val);
//    else if (a.type == "this")
//      val = this.resource;
      else
        val = gRDF.GetResource(val);
      
      var old = this.datasources[a.dsid].GetTarget(this.resource, prop, true);
      if (old) {
        if (old.Value == val.Value)
          continue;
        // ... else
        this.datasources[a.dsid].Change(this.resource, prop, old, val, true);
      }
      else
        this.datasources[a.dsid].Assert(this.resource, prop, val, true);
      this._executeTriggers(a, val);
    }
    if (this.autoflush)
      this.flush();
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
    if (this.removeHook)
      this.removeHook();
    
    if (this.autoflush)
      this.flush();
    this.resource = null;
  });

// should we automatically flush associated datasources whenever data
// is being set?
PersistentRDFObject.obj("autoflush", true);

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
