/* 
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla Session Roaming code.
 * 
 * The Initial Developer of the Original Code is
 * Ben Bucksch <http://www.bucksch.org> of
 * Beonex <http://www.beonex.com>
 * Portions created by Ben Bucksch are Copyright (C) 2002 Ben Bucksch.
 * All Rights Reserved.
 * 
 * Contributor(s): 
 *    Netscape Editor team, esp. Charles Manske <cmanske@netscape.com>
 *    ManyOne <http://www.manyone.net>
 */

/* Provides some global functions useful for implementing the backend or UI
   of the file transfer.
*/

////////////////////////////////////////////////////////////////////////////
// UI
////////////////////////////////////////////////////////////////////////////



var gStringBundle;
function GetString(name)
{
  if (!gStringBundle)
  {
    try {
      var strBundleService =
          Components.classes["@mozilla.org/intl/stringbundle;1"].getService(); 
      strBundleService = 
          strBundleService.QueryInterface(Components.interfaces.nsIStringBundleService);

      gStringBundle = strBundleService.createBundle("chrome://sroaming/locale/transfer.properties");

    } catch (ex) {}
  }
  if (gStringBundle)
  {
    try {
      return gStringBundle.GetStringFromName(name);
    } catch (e) {}
  }
  return null;
}

function GetStringWithFile(stringname, filename)
{
  return GetString(stringname).replace(/%file%/, filename);
}

/* Translates the result of a file transfer into a String for display
   to the user.

   @param aFile  Object from transfer.files[]
   @return String
*/
function ErrorMessageForFile(aFile)
{
  if (aFile.statusCode == kStatusHTTP)
    return GetStringWithFile("HTTPResponse", aFile.filename)
           .replace(/%responsecode%/, aFile.httpResponse)
           .replace(/%responsetext%/, aFile.statusText);
  else
    return ErrorMessageForStatusCode(aFile.statusCode, aFile.filename);
}

/* Translates an XPCOM result code into a String for display to the user.

   @param aStatusCode  int  XPCOM result code
   @param aFilename  String  relative filename
   @return String
*/
function ErrorMessageForStatusCode(aStatusCode, aFilename)
{
  var statusMessage = null;
  switch (aStatusCode)
  {
    case 0:
      statusMessage = GetStringWithFile("NSOK", aFilename);
      break;
    case kFileNotFound:
      gFileNotFound = true;
      statusMessage = GetStringWithFile("FileNotFound", aFilename);
      break;
    case kNetReset:
      // We get this when subdir doesn't exist AND
      //   if aFilename used is same as an existing subdir 
      var dir = (gTransferData.filename == aFilename) ? gTransferData.docDir
                                                      : gTransferData.otherDir;

      if (dir)
      {
        // This is the ambiguous case when we can't tell,
        // if subdir or filename is bad
        statusMessage = GetString("SubdirDoesNotExist").replace(/%dir%/, dir);
        statusMessage = statusMessage.replace(/%file%/, aFilename);

        // Remove directory from saved prefs
        // XXX Note that if subdir is good, 
        //     but filename = next level subdirectory name, 
        //     we really shouldn't remove subdirectory, 
        //     but it's impossible to differentiate this case!
        RemoveTransferSubdirectoryFromPrefs(gTransferData, dir);
      }
      else
        statusMessage = GetStringWithFile("FilenameIsSubdir", aFilename);
      break;
    case kNotConnected:
    case kConnectionRefused:
    case kNetTimeout:
    case kNoConnectionOrTimeout:
    case kPortAccessNotAllowed:
      statusMessage = GetString("ServerNotAvailable");
      break;
    case kOffline:
      statusMessage = GetString("Offline");
      break;
    case kDiskFull:
    case kNoDeviceSpace:
      statusMessage = GetStringWithFile("DiskFull", aFilename);
      break;
    case kNameTooLong:
      statusMessage = GetStringWithFile("NameTooLong", aFilename);
      break;
    case kAccessDenied:
      statusMessage = GetStringWithFile("AccessDenied", aFilename);
      break;
    case kErrorAbort:
      statusMessage = GetStringWithFile("Abort");
      break;
    case kUnknownType:
    default:
      statusMessage = GetStringWithFile("UnknownTransferError", aFilename)
                                        + " " + NameForStatusCode(aStatusCode);
      break;
  }
  return statusMessage
}


////////////////////////////////////////////////////////////////////////////
// Error handling (general)
////////////////////////////////////////////////////////////////////////////

/*
  This is bad. We should have this in a central place, but I don't know
  (nor could find) one where this is available. Note that I do use
  |Components.results| in the end, but it doesn't have all the errors I need.
 */

// Transfer error codes
//   These are translated from C++ error code strings like this:
//   kFileNotFound = "NS_FILE_NOT_FOUND",
const kNS_OK = 0;
const kNetBase = 2152398848;
const kFilesBase = 2152857600;
const kUnknownType = kFilesBase + 04; // nsError.h
const kDiskFull = kFilesBase + 10; // nsError.h
const kNoDeviceSpace = kFilesBase + 16; // nsError.h
const kNameTooLong = kFilesBase + 17; // nsError.h
const kFileNotFound = kFilesBase + 18; // nsError.h, 0x80520012
const kAccessDenied = kFilesBase + 21; // nsError.h
const kNetReset = kNetBase + 20;
const kNotConnected = kNetBase + 12; // netCore.h
const kConnectionRefused = kNetBase + 13;
const kNetTimeout = kNetBase + 14;
const kInProgress = kNetBase + 15; // netCore.h
const kOffline = kNetBase + 16; // netCore.h; 0x804b0010
const kNoConnectionOrTimeout = kNetBase + 30;
const kPortAccessNotAllowed = kNetBase + 19; // netCore.h
const kErrorBindingFailed = kNetBase + 1; // netCore.h
const kErrorBindingAborted = kNetBase + 2; // netCore.h
const kErrorBindingRedirected = kNetBase + 3; // netCore.h
const kErrorBindingRetargeted = kNetBase + 4; // netCore.h
const kStatusResolvingHost = kNetBase + 80;// nsISocketTransport.idl
const kStatusConnectedTo = kNetBase + 81; // nsISocketTransport.idl
const kStatusSendingTo = kNetBase + 5; // nsISocketTransport.idl
const kStatusRecievingFrom = kNetBase + 6; // nsISocketTransport.idl
const kStatusConnectingTo = kNetBase + 7; // nsISocketTransport.idl
const kStatusWaitingFor = kNetBase + 82 // nsISocketTransport.idl
const kStatusReadFrom = kNetBase + 8; // nsIFileTransportService.idl
const kStatusBeginFTPTransaction = kNetBase + 27; // ftpCore.h
const kStatusEndFTPTransaction = kNetBase + 28; // ftpCore.h
const kStatusFTPLogin = kNetBase + 21; // ftpCore.h
const kStatusFTPCWD = kNetBase + 22; // ftpCore.h
const kStatusFTPPassive = kNetBase + 23; // ftpCore.h
const kStatusFTPPWD = kNetBase + 24; // ftpCore.h
const kErrorNoInterface = 0x80004002; // nsError.h
const kErrorNotImplemented = 0x80004001; // nsError.h
const kErrorAbort = 0x80004004; // nsError.h
const kErrorFailure = 0x80004005; // nsError.h
const kErrorUnexpected = 0x8000ffff; // nsError.h
const kErrorInvalidValue = 0x80070057; // nsError.h
const kErrorNotAvailable = 0x80040111; // nsError.h
const kErrorNotInited = 0xC1F30001; // nsError.h
const kErrorAlreadyInited = 0xC1F30002; // nsError.h
const kErrorFTPAuthNeeded = 0x4B001B; // XXX not sure what exactly this is or
   // where it comes from (grep doesn't find it in dec or hex notation), but
   // that's what I get when the credentials are not accepted by the FTP server
const kErrorFTPAuthFailed = 0x4B001C; // dito
const kStatusHTTP = 500; // XXX

// Translates an XPCOM result code into a String similar to the C++ constant.
function NameForStatusCode(aStatusCode)
{
  if (aStatusCode == 0)
    return "NS_OK";
  else if (aStatusCode == kStatusReadFrom)
    return "NET_STATUS_READ_FROM";
  else if (aStatusCode == kStatusRecievingFrom)
    return "NET_STATUS_RECEIVING_FROM";
  else if (aStatusCode == kStatusSendingTo)
    return "NET_STATUS_SENDING_TO";
  else if (aStatusCode == kStatusWaitingFor)
    return "NET_STATUS_WAITING_FOR";
  else if (aStatusCode == kStatusHTTP)
    return "See HTTP response";
  else if (aStatusCode == kStatusResolvingHost)
    return "NET_STATUS_RESOLVING_HOST";
  else if (aStatusCode == kStatusConnectedTo)
    return "NET_STATUS_CONNECTED_TO";
  else if (aStatusCode == kStatusConnectingTo)
    return "NET_STATUS_CONNECTING_TO";
  else if (aStatusCode == kErrorBindingFailed)
    return "BINDING_FAILED";
  else if (aStatusCode == kErrorBindingAborted)
    return "BINDING_ABORTED";
  else if (aStatusCode == kErrorBindingRedirected)
    return "BINDING_REDIRECTED";
  else if (aStatusCode == kErrorBindingRetargeted)
    return "BINDING_RETARGETED";
  else if (aStatusCode == kNetBase + 10) // netCore.h
    return "MALFORMED_URI";
  else if (aStatusCode == kNetBase + 11) // netCore.h
    return "ALREADY_CONNECTED";
  else if (aStatusCode == kNotConnected)
    return "NOT_CONNECTED";
  else if (aStatusCode == kConnectionRefused)
    return "CONNECTION_REFUSED";
  else if (aStatusCode == kNetTimeout)
    return "NET_TIMEOUT";
  else if (aStatusCode == kInProgress)
    return "IN_PROGRESS";
  else if (aStatusCode == kOffline)
    return "OFFLINE";
  else if (aStatusCode == kNetBase + 17) // netCore.h
    return "NO_CONTENT";
  else if (aStatusCode == kNetBase + 18) // netCore.h
    return "UNKNOWN_PROTOCOL";
  else if (aStatusCode == kPortAccessNotAllowed)
    return "PORT_ACCESS_NOT_ALLOWED";
  else if (aStatusCode == kNetReset)
    return "NET_RESET";
  else if (aStatusCode == kStatusFTPLogin)
    return "FTP_LOGIN";
  else if (aStatusCode == kStatusFTPCWD)
    return "FTP_CWD";
  else if (aStatusCode == kStatusFTPPassive)
    return "FTP_PASV";
  else if (aStatusCode == kStatusFTPPWD)
    return "FTP_PWD";
  else if (aStatusCode == kStatusBeginFTPTransaction)
    return "NET_STATUS_BEGIN_FTP_TRANSACTION ";
  else if (aStatusCode == kStatusEndFTPTransaction)
    return "NET_STATUS_END_FTP_TRANSACTION";
  else if (aStatusCode == kFilesBase + 1) // nsError.h
    return "UNRECOGNIZED_PATH";
  else if (aStatusCode == kFilesBase + 2) // nsError.h
    return "UNRESOLABLE SYMLINK";
  else if (aStatusCode == kFilesBase + 4) // nsError.h
    return "UNKNOWN_TYPE";
  else if (aStatusCode == kFilesBase + 5) // nsError.h
    return "DESTINATION_NOT_DIR";
  else if (aStatusCode == kFilesBase + 6) // nsError.h
    return "TARGET_DOES_NOT_EXIST";
  else if (aStatusCode == kFilesBase + 8) // nsError.h
    return "ALREADY_EXISTS";
  else if (aStatusCode == kFilesBase + 9) // nsError.h
    return "INVALID_PATH";
  else if (aStatusCode == kDiskFull)
    return "DISK_FULL";
  else if (aStatusCode == kFilesBase + 11) // nsError.h
    return "FILE_CORRUPTED (justice department, too)";
  else if (aStatusCode == kFilesBase + 12) // nsError.h
    return "NOT_DIRECTORY";
  else if (aStatusCode == kFilesBase + 13) // nsError.h
    return "IS_DIRECTORY";
  else if (aStatusCode == kFilesBase + 14) // nsError.h
    return "IS_LOCKED";
  else if (aStatusCode == kFilesBase + 15) // nsError.h
    return "TOO_BIG";
  else if (aStatusCode == kNoDeviceSpace)
    return "NO_DEVICE_SPACE";
  else if (aStatusCode == kNameTooLong)
    return "NAME_TOO_LONG";
  else if (aStatusCode == kFileNotFound)
    return "FILE_NOT_FOUND";
  else if (aStatusCode == kFilesBase + 19) // nsError.h
    return "READ_ONLY";
  else if (aStatusCode == kFilesBase + 20) // nsError.h
    return "DIR_NOT_EMPTY";
  else if (aStatusCode == kAccessDenied)
    return "ACCESS_DENIED";
  else if (aStatusCode == kNoConnectionOrTimeout)
    return "NO_CONNECTION_OR_TIMEOUT";
  else if (aStatusCode == kErrorFTPAuthNeeded)
    return "FTP auth needed ?";
  else if (aStatusCode == kErrorFTPAuthFailed)
    return "FTP auth failed ?";
  else
    return prettyError(aStatusCode);
}

function prettyError(num)
{
  for (a in Components.results)
    if (Components.results[a]==num)
      return a;
  return String(num);
}







////////////////////////////////////////////////////////////////////////////
// Debugging (backend and UI)
////////////////////////////////////////////////////////////////////////////




// Turn this on to get debug output.
const debugOutput = true;
function ddump(text) {
  if (debugOutput) {
    dump(text + "\n");
  }
}
function ddumpCont(text) {
  if (debugOutput) {
    dump(text);
  }
}
function dumpObject(obj, name, maxDepth, curDepth) {
  if (curDepth == undefined)
    curDepth = 0;
  if (maxDepth != undefined && curDepth > maxDepth)
    return;

  var i = 0;
  for (prop in obj) {
    i++;
    if (typeof(obj[prop]) == "object")
    {
      if (obj[prop] && obj[prop].length != undefined)
        ddump(name + "." + prop + "=[probably array, length "
              + obj[prop].length + "]");
      else
        ddump(name + "." + prop + "=[" + typeof(obj[prop]) + "]");
      dumpObject(obj[prop], name + "." + prop, maxDepth, curDepth+1);
    }
    else if (typeof(obj[prop]) == "function")
      ddump(name + "." + prop + "=[function]");
    else
      ddump(name + "." + prop + "=" + obj[prop]);
  }
  if (!i)
    ddump(name + " is empty");    
}
function dumpError(text) {
  dump(text + "\n");
}
