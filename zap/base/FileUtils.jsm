/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/FileUtils.jsm', null)" -*- */
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

EXPORTED_SYMBOLS = [ "urlToFile",
                     "fileToURL",
                     "openFileForWriting",
                     "write16LE",
                     "write32LE",
                     "openFileForReading",
                     "readFile",
                     "getProfileDir",
                     "getProfileFile",
                     "getProfileDefaultsFile",
                     "ensureProfileFile",
                     "getProfileFileURL",
                     "getStageDir",
                     "getStageFile"
                   ];


// name our global object:
// function toString() { return "[FileUtils.jsm]"; }


////////////////////////////////////////////////////////////////////////
// Globals

var gDirectoryService = Components.classes["@mozilla.org/file/directory_service;1"].getService(Components.interfaces.nsIProperties);
var gIOService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
var gFileProtocolHandler = gIOService.getProtocolHandler("file").QueryInterface(Components.interfaces.nsIFileProtocolHandler);

// ioflags for nsIFileOutputStream::init()
var IO_RDONLY               = 0x01;
var IO_WRONLY               = 0x02;
var IO_RDWR                 = 0x04;
var IO_CREATE_FILE          = 0x08;
var IO_APPEND               = 0x10;
var IO_TRUNCATE             = 0x20;
var IO_SYNC                 = 0x40;
var IO_EXCL                 = 0x80;

// file mode bits for nsIFileOutputStream::init() (taken from prio.h):
var IO_IRUSR = 00400;  /* read permission, owner */
var IO_IWUSR = 00200;  /* write permission, owner */
var IO_IXUSR = 00100;  /* execute/search permission, owner */
var IO_IRGRP = 00040;  /* read permission, group */
var IO_IWGRP = 00020;  /* write permission, group */
var IO_IXGRP = 00010;  /* execute/search permission, group */
var IO_IROTH = 00004;  /* read permission, others */
var IO_IWOTH = 00002;  /* write permission, others */
var IO_IXOTH = 00001;  /* execute/search permission, others */


////////////////////////////////////////////////////////////////////////
//

defun(
  "convert a url string to an nsIFile object",
  function urlToFile(url) {
    try {
      return gIOService.newURI(url, null, null)
        .QueryInterface(Components.interfaces.nsIFileURL).file;
    }
    catch(e) {
      // doesn't appear to be a file
      return null;
    }
  });


defun(
  "convert an nsIFile object to a url string.",
  function fileToURL(file) {
    return gFileProtocolHandler.getURLSpecFromFile(file);
  });

defun(
  "Open a file for overwriting or appending. Returns an "+
  "nsIBinaryOutputStream object. Permissions should be a 9 character "+
  "string describing the permissions as 'ls -l' would display "+
  "them. Default permissions are 'rw-r--r--'.",
  function openFileForWriting(file, append, /* optional */ permissions)
  {
    var fos;
    var perms = 0;
    if (permissions) {
      if (permissions[0] == "r") perms |= IO_IRUSR;
      if (permissions[1] == "w") perms |= IO_IWUSR;
      if (permissions[2] == "x") perms |= IO_IXUSR;
      if (permissions[3] == "r") perms |= IO_IRGRP;
      if (permissions[4] == "w") perms |= IO_IWGRP;
      if (permissions[5] == "x") perms |= IO_IXGRP;
      if (permissions[6] == "r") perms |= IO_IROTH;
      if (permissions[7] == "w") perms |= IO_IWOTH;
      if (permissions[8] == "x") perms |= IO_IXOTH;
    }
    else
      perms = IO_IRUSR | IO_IWUSR | IO_IRGRP | IO_IROTH;
    
    try {
      fos = Components.classes["@mozilla.org/network/file-output-stream;1"]
        .createInstance(Components.interfaces.nsIFileOutputStream);
      var ioflags = IO_WRONLY | IO_CREATE_FILE;
      if (append)
        ioflags |= IO_APPEND;
      else
        ioflags |= IO_TRUNCATE;
      fos.init(file, ioflags, perms, null);

      var bos = Components.classes["@mozilla.org/binaryoutputstream;1"]
        .createInstance(Components.interfaces.nsIBinaryOutputStream);
      bos.setOutputStream(fos);
      fos = bos;
    }
    catch(e) {
      if (fos)
        fos.close();
      fos = null;
    }
    return fos;
  });

defun(
  "Writes the given 16 bit integer 'x' to the given output stream in "+
  "little-endian order (low-order byte first)",
  function write16LE(x, stream) {
    stream.write8(x);
    stream.write8(x>>8);
  });

defun(
  "Writes the given 32 bit integer 'x' to the given output stream in "+
  "little-endian order (low-order byte first)",
  function write32LE(x, stream) {
    stream.write8(x);
    stream.write8(x>>8);
    stream.write8(x>>16);
    stream.write8(x>>24);
  });

defun(
  "Open a file for reading. Returns an nsIScriptableInputStreamEx object.",
  function openFileForReading(file) {
    try {
      var fis = Components.classes["@mozilla.org/network/file-input-stream;1"]
        .createInstance(Components.interfaces.nsIFileInputStream);
      var ioflags = IO_RDONLY;
      fis.init(file, ioflags, 0, 0);
    }
    catch(e) {
      if (fis)
        fis.close();
      fis = null;
    }
    var sis = Components.classes["@mozilla.org/scriptableinputstream;1"].createInstance(Components.interfaces.nsIScriptableInputStreamEx);
    sis.init(fis);
    return sis;  
  });

defun(
  "Returns a string with the content of the given file. "+
  "Throws an exception if the file can't be read.",
  function readFile(file) {
    var sis = openFileForReading(file);
    if (!sis) throw("Can't open file");
    var rv = "";
    var avail;
    while ((avail = sis.available()))
      rv += sis.readEx(avail);
    
    return rv;
  });

defun(
  "returns the profile directory",
  function getProfileDir()
  {
    return gDirectoryService.get("ProfD", Components.interfaces.nsIFile);
  });

defun(
  "get a file in the profile directory",
  function getProfileFile(name)
  {
    var file = gDirectoryService.get("ProfD",
                                     Components.interfaces.nsIFile);
    file.append(name);
    return file;
  });

defun(
  "get a file in the profile defaults directory",
  function getProfileDefaultsFile(name)
  {
    var file = gDirectoryService.get("ProfDefNoLoc",
                                     Components.interfaces.nsIFile);
    file.append(name);
    return file;
  });

defun(
  "get the url of a file in the profile directory (e.g. for opening a "+
  "datasource)",
  function getProfileFileURL(name)
  {
    return gFileProtocolHandler.getURLSpecFromFile(getProfileFile(name));
  });

defun(
  "Ensure the given profile file exists; copying it from the profile "+
  "defaults directory if necessary. Returns 'false' if the file "+
  "doesn't exist or can't be copied; 'true' otherwise. ",
  function ensureProfileFile(name)
  {
    var file = getProfileFile(name);
    if (file.exists()) return true;
    
    // file doesn't exist. try copying from profile defaults directory:
    var dfile = getProfileDefaultsFile(name);
    if (!dfile.exists()) return false;
    try {
      dfile.copyTo(getProfileDir(), "");
    }
    catch(e) {
      return false;
    }
    return true;
  });

defun(
  "returns the xulrunner staging directory",
  function getStageDir()
  {
    // XXX is XCurProcD correct here?
    return gDirectoryService.get("XCurProcD", Components.interfaces.nsIFile);
  });

defun(
  "get a file in the staging directory",
  function getStageFile(name)
  {
    // XXX is XCurProcD correct here?
    var file = gDirectoryService.get("XCurProcD",
                                     Components.interfaces.nsIFile);
    file.append(name);
    return file;
  });
