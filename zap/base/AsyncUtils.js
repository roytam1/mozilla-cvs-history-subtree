/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.utils.importModule('gre:AsyncUtils.js', null)" -*- */
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

Components.utils.importModule("gre:FunctionUtils.js");


EXPORTED_SYMBOLS = [ "getMainThread",
                     "makeOneShotTimer",
                     "resetOneShotTimer",
                     "schedule",
                     "schedulePeriodic",
                     "callAsync"];

// object to hold module's documentation:
var _doc_ = {};


////////////////////////////////////////////////////////////////////////
// Threads

var getThreadManager = makeServiceGetter("@mozilla.org/thread-manager;1",
                                         Components.interfaces.nsIThreadManager);

function getMainThread() {
  return getThreadManager().mainThread;
}

////////////////////////////////////////////////////////////////////////
// Timer utilities:

var CLASS_TIMER = "@mozilla.org/timer;1";

/** Create a new one-shot timer
 *
 * 'callback' must implement nsITimerCallback
 * 'duration' is the time in ms after which the timer fires
 *
 * The timer can be cancelled with timer.cancel() and reset with a
 * different duration using resetOneShotTimer(.).
 */
function makeOneShotTimer(callback, duration) {
  // We need to use 'this.Components' to ensure that xpconnect wraps
  // the timer for the caller's global object (assuming that the
  // callback has the same global object). See also the comments in
  // ClassUtils.js, function makeClass().
  var timer = this.Components.classes[CLASS_TIMER].createInstance(this.Components.interfaces.nsITimer);
  timer.initWithCallback(callback, duration, this.Components.interfaces.nsITimer.TYPE_ONE_SHOT);
  return timer;
}

function resetOneShotTimer(timer, duration) {
  timer.initWithCallback(timer.callback, duration, this.Components.interfaces.nsITimer.TYPE_ONE_SHOT);
  return timer;
}

// Asynchronously call 'fct' after 'interval' ms. Return timer
// object. Calling 'cancel()' on the return value will cancel the
// scheduled function call (or any subsequent re-schedules).
// fct will be called with a function argument 'reschedule(interval)',
// which can be used to schedule another call of 'fct'
function schedule(fct, interval) {
  return makeOneShotTimer(
    {
      notify : function(timer) {
        fct(function reschedule(_interval) {
              resetOneShotTimer(timer, _interval ? _interval : interval);
            });
      }
    },
    interval);
}

// helper for schedulePeriodic:
function callback(fct) {
  this.fct = fct;
}
callback.prototype.notify = function(t) { this.fct(); };

// Periodically make an asynchronous call to 'fct' every 'period'
// ms. Return timer object. Calling 'cancel()' on the return value
// will cancel the schedule.
function schedulePeriodic(fct, period) {
  var timer = this.Components.classes[CLASS_TIMER].createInstance(this.Components.interfaces.nsITimer);
  // to prevent a reference cycle it is important here that the
  // callback object is not created with the 'schedulePeriodic'
  // closure (because this closure will keep the timer alive through
  // the 'timer' variable).
  timer.initWithCallback(new callback(fct),
                         period,
                         this.Components.interfaces.nsITimer.TYPE_REPEATING_SLACK);
  return timer;
}


// Call 'fct' asynchronously:
function callAsync(fct) {
  schedule(fct, 0);
//   // XXX we *really* want a new method on nsIEventTarget for posting
//   // from JS here. setTimeout is not a good idea, since, among other
//   // things, it sets up a 10ms timer even if the timeout value is 0ms.
//   // This means that there is the potential for race conditions - the
//   // call is not guaranteed to be the next one in the queue.
//   var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
//                      .getService(Components.interfaces.nsIWindowMediator);
//   var enumerator = wm.getEnumerator(null);
//   var retval = [];
//   if (!enumerator.hasMoreElements()) return; // can't post
//   enumerator.getNext().setTimeout(fct, 0);
}
