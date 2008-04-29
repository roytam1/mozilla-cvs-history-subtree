/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/AsyncUtils.jsm', null)" -*- */
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

Components.utils.import("resource://gre/components/FunctionUtils.jsm");


EXPORTED_SYMBOLS = [ "getMainThread",
                     "getSyncProxyOnMainThread",
                     "getAsyncProxyOnMainThread",
                     "makeOneShotTimer",
                     "resetOneShotTimer",
                     "schedule",
                     "schedulePeriodic",
                     "callAsync"];

////////////////////////////////////////////////////////////////////////
// Threads

var getThreadManager = makeServiceGetter("@mozilla.org/thread-manager;1",
                                         Components.interfaces.nsIThreadManager);

var IProxyManager = Components.interfaces.nsIProxyObjectManager;
var getProxyManager = makeServiceGetter("@mozilla.org/xpcomproxy;1",
                                        IProxyManager);

defun(
  function getMainThread() {
    return getThreadManager().mainThread;
  });

defun(
  function getSyncProxyOnMainThread(obj, itf) {
    return getProxyManager().getProxyForObject(getMainThread(),
                                               itf, obj,
                                               IProxyManager.INVOKE_SYNC |
                                               IProxyManager.FORCE_PROXY_CREATION);
  });

defun(
  function getAsyncProxyOnMainThread(obj, itf) {
    return getProxyManager().getProxyForObject(getMainThread(),
                                               itf, obj,
                                               IProxyManager.INVOKE_ASYNC |
                                               IProxyManager.FORCE_PROXY_CREATION);
  });

////////////////////////////////////////////////////////////////////////
// Timer utilities:

var CLASS_TIMER = "@mozilla.org/timer;1";

defun(
  "Create a new one-shot timer\n"+
  "  'callback' must implement nsITimerCallback\n"+
  "  'duration' is the time in ms after which the timer fires\n"+
  "The timer can be cancelled with timer.cancel() and reset with a "+
  "different duration using resetOneShotTimer(.).",
  function makeOneShotTimer(callback, duration) {
    // We need to use 'this.Components' to ensure that xpconnect wraps
    // the timer for the caller's global object (assuming that the
    // callback has the same global object). See also the comments in
    // ClassUtils.jsm, function makeClass().
    var timer = this.Components.classes[CLASS_TIMER].createInstance(this.Components.interfaces.nsITimer);
    timer.initWithCallback(callback, duration, this.Components.interfaces.nsITimer.TYPE_ONE_SHOT);
    return timer;
  });

defun(
  function resetOneShotTimer(timer, duration) {
    timer.initWithCallback(timer.callback, duration, this.Components.interfaces.nsITimer.TYPE_ONE_SHOT);
    return timer;
  });

defun(
  "Asynchronously call 'fct' after 'interval' ms. ",
  function schedule(fct, interval) {
    // We'll hold onto the timer object in the closure to prevent it
    // from being garbarge collected.
    var timer = makeOneShotTimer(
      {
        notify : function(t) {
          fct();
          // Now make sure that the timer can be garbage collected:
          timer = null;
        }
      },
      interval);
  });

// helper for schedulePeriodic:
function callback(fct) {
  this.fct = fct;
}
callback.prototype.notify = function(t) { this.fct(); };

defun(
  "Periodically make an asynchronous call to 'fct' every 'period' "+
  "ms. Return timer object. Calling 'cancel()' on the return value "+
  "will cancel the schedule.\n"+
  "The caller must hold onto the timer object for as long as the timer is "+
  "supposed to run (otherwise it might be garbage collected).",
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
  });

// helper for callAsync:
function event(fct) {
  this.run = fct;
}

defun(
  "Call 'fct' asynchronously.",
  function callAsync(fct) {
    getMainThread().dispatch(new event(fct), 0);
  });
