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
 * The Initial Developer of the Original Code is (XXX am I?)
 *        Ben Bucksch <http://www.bucksch.org>
 *        of Beonex <http://www.beonex.com>
 * Portions created by Ben Bucksch are Copyright (C) 2002 Ben Bucksch.
 * All Rights Reserved.
 * 
 * Contributor(s): 
 *    Netscape Editor team, esp. Charles Manske <cmanske@netscape.com>
 */

/* Displays the progress of the down/upload of profile files to an
   HTTP/FTP server.
   It also initiates the actual transfer, using transfer.js,
   see mozSRoamingStream.cpp for the reasons.
*/

var gTransfer;
var gDialog = {};
var gInProgress = true;
var gTotalFileCount = 0;
var gSucceededCount = 0;
var gFinished = false;
var gTransferingFailed = false;
var gFileNotFound = false;
var gStatusMessage="";

var gTimerID;
var gTimeout = 1000;
var gAllowEnterKey = false;

const XUL_NS ="http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";


function Startup()
{
  ddump("In sroaming/transfer/progressDialog::Startup()");

  try
  {
    GetParams(); // dialog params -> gTransfer
    //gTransfer.transfer(); // non-blocking
    checkAndTransfer(gTransfer, null); // half-blocking
  }
  catch (e)
  {
    /* All kinds of exceptions should end up here, esp. from init, meaning
       this is the main last backstop for unexpected or fatal errors,
       esp. those not related to a certain file. */
    SetGlobalStatusMessage(ErrorMessageForException(e));
  	return;
  }

  gDialog.FileList           = document.getElementById("FileList");
  gDialog.FinalStatusMessage = document.getElementById("FinalStatusMessage");
  gDialog.StatusMessage      = document.getElementById("StatusMessage");
  gDialog.Close              = document.documentElement.getButton("cancel");

  SetWindowLocation();
  window.title = GetString("TransferProgressCaption");

  var directionString = gTransfer.download
                        ? GetString("TransferFromSite")
                        : GetString("TransferToSite");
  document.getElementById("TransferToFromSite").value = 
                      directionString.replace(/%site%/, gTransfer.sitename);

  // Show transfering destination URL
  document.getElementById("TransferUrl").value = gTransfer.remoteDir;

  // Add the files to the "transfer to" list as quick as possible!
  gDialog.FileList.setAttribute("rows", gTransfer.files.length);
  for (var i = 0; i < gTransfer.files.length; i++)
    SetProgressStatus(i);
  window.sizeToContent();
  ddump("resized");
}

/*
  Reads in the params we got from the calling function. Creates gTransfer.
*/
function GetParams()
{
  var params = window.arguments[0].QueryInterface(
                                   Components.interfaces.nsIDialogParamBlock);
  /* For definition of meaning of params, see
     mozSRoamingStream.cpp::DownUpLoad() */

  // download
  var direction = params.GetInt(0);
  ddump("Passing in: Int 0 (direction) is " + direction);
  if (direction != 1 && direction != 2)
    throw "Error: Bad direction param";
  var download = direction == 1;

  // serial/parallel
  var serial_param = params.GetInt(1);
  ddump("Passing in: Int 1 (serial) is " + serial_param);
  if (serial_param != 1 && serial_param != 2)
    throw "Error: Bad serial param";
  var serial = serial_param == 1;
  serial = true;

  // files count
  var count = params.GetInt(2);
  ddump("Passing in: Int 2 (files count) is " + count);
  if (count == 0)
    throw GetString("NoFilesSelected");
  if (count < 0)
    throw "Error: Bad count param";

  // save pw
  var savepw_param = params.GetInt(3);
  ddump("Passing in: Int 3 (save pw) is " + savepw_param);
  var savepw = false;
  if (savepw_param == 1)
    savepw = true;

  // profile dir
  var profileDir = params.GetString(1);
  ddump("Passing in: String 1 (profile dir) is " + profileDir);
  if (profileDir == null || profileDir == "")
    throw "Error: Bad profile dir param";

  // remote dir
  var remoteDir = params.GetString(2);
  ddump("Passing in: String 2 (remote dir) is " + remoteDir);
  if (remoteDir == null || remoteDir == "")
    throw "Error: Bad remote dir param";

  // password
  var password = params.GetString(3);
  ddump("Passing in: String 3 (password) is " + password);
  // ignore errors

  // filenames
  var files = new Array();
  for (var i = 0; i < count; i++)
  {
    var filename = params.GetString(i + 4);  // filenames start at item 4
    ddumpCont("Passing in: String " + (i + 4));
    ddump(" (file " + i + ") is " + filename);
    if (filename == null || filename == "")
      throw "Error: Bad filename";
    files[i] = new Object();
    files[i].filename = filename;
    files[i].mimetype = undefined;
    files[i].size = undefined;
  }

  gTransfer = new Transfer(download, serial,
                           profileDir, remoteDir,
                           password, savepw,
                           files,
                           undefined, SetProgressStatus);
}

function PassBackParams()
{
  ddump("PassBackParam()");
  var params = window.arguments[0].QueryInterface(
                                    Components.interfaces.nsIDialogParamBlock);
  if (gTransfer)
  {
    params.SetInt(0, gTransfer.getSaveLogin());
    ddump(" int0: " + gTransfer.getSaveLogin());
    if (gTransfer.getSaveLogin() == 1)
    {
      params.SetString(0, gTransfer.getUsername());
      params.SetString(1, gTransfer.getPassword());
      ddump(" string0: " + gTransfer.getUsername());
      ddump(" string1: " + gTransfer.getPassword());
    }
  }
  else /* e.g. if we didn't get good prefs and couldn't create a transfer
          in the first place */
  {
    params.SetInt(0, 0);
    params.SetString(0, "");
    params.SetString(1, "");
  }
  // XXX call gTransfer.done()?
  ddump(" done");
}



/* Add filename to list of files to transfer
   or set status for file already in the list.

   @param filei  integer  index of file in gTransfer
   @return boolean  if file was in the list
*/
function SetProgressStatus(filei)
{
  ddumpCont("SetProgressStatus(" + filei + "): ");
  filename = gTransfer.files[filei].filename;
  status = gTransfer.files[filei].status;
  statusCode = gTransfer.files[filei].statusCode;
  ddump(filename + ", " + status + ", " + NameForStatusCode(statusCode));

  if (!filename)
    return false;

  // Output error msg, if apppropriate
  if (status == "failed")
  {
    SetFileStatusMessage(filei, ErrorMessageForFile(gTransfer.files[filei]));
  }

  // Just set attribute for status icon 
  // if we already have this filename 
  var listitems = document.getElementsByTagName("listitem");
  for (var i = 0; i < listitems.length; i++)
  {
  	var li = listitems[i];
    if (li.getAttribute("label") == filename)
    {
      if (li.getAttribute("progress") != status)
      {
        var oldstat = li.getAttribute("progress");
        ddump("  Setting "+filename+" from "+oldstat+" to "+status); 
        li.setAttribute("progress", status);
        CloseIfPossible();
      }
      return true;
    }
  }

  // If we didn't return yet, we're adding a new file item to list
  gTotalFileCount++;

  var listitem = document.createElementNS(XUL_NS, "listitem");
  if (listitem)
  {
    listitem.setAttribute("class", "listitem-iconic progressitem");
    // This triggers CSS to show icon for each status state
    listitem.setAttribute("progress", status);
    listitem.setAttribute("label", filename);
    gDialog.FileList.appendChild(listitem);
  }
  return false;
}

function SetProgressFinished(filei)
{
  ddumpCont("SetProgressFinished(" + filei + "): ");
  filename = gTransfer.files[filei].filename;
  networkStatus = gTransfer.files[filei].statusCode;
  status = gTransfer.files[filei].status;
  ddump(filename + ", " + status);

  SetProgressStatus(filei);

  if (status == "done")
    gSucceededCount++;
  else // Error condition
  {
    // Mark all remaining files as "failed" XXX wrong
    gTransferingFailed = true;
    SetProgressStatusCancel();
    gDialog.FinalStatusMessage.value = GetString("TransferFailed");

    gStatusMessage = ErrorMessageForFile(gTransfer.files[filei]);
  }

  if (gStatusMessage)
    SetFileStatusMessage(filei, gStatusMessage);
}

function SetProgressFinishedAll()
{
    gDialog.Close.setAttribute("label", GetString("Close"));
    if (!gStatusMessage)
      gStatusMessage = GetString(gTransferingFailed
                                 ? "UnknownTransferError" :
                                 "AllFilesTransfered");

    // Now allow "Enter/Return" key to close the dialog
    AllowDefaultButton();

    if (gTransferingFailed || gFileNotFound)
    {
      // Show "Troubleshooting" button to help solving problems
      //  and key for successful / failed files
      document.getElementById("failureBox").hidden = false;
    }
}

// this function is to be used when we cancel persist's saving,
// because not all messages will be returned to us, if we cancel.
// this function changes status for all non-done/non-failure to failure.
function SetProgressStatusCancel()
{
  var listitems = document.getElementsByTagName("listitem");
  if (!listitems)
    return;

  for (var i=0; i < listitems.length; i++)
  {
    var attr = listitems[i].getAttribute("progress");
    if (attr != "done" && attr != "failed")
      listitems[i].setAttribute("progress", "failed");
  }
}

function CloseIfPossible()
{
  ddump("CloseIfPossible()");
  for (var i = 0; i < gTransfer.files.length; i++)
  {
    ddumpCont("  Checking " + i + ", " + gTransfer.files[i].filename + ", ");
    ddump(gTransfer.files[i].status);
    if (gTransfer.files[i].status != "done")  // do not close, if failed
      return;
  }
  ddump("  Closing");

  // Finish progress messages, settings buttons etc.
  SetProgressFinishedAll();

  // Set "completed" message if we succeeded
  if (!gTransferingFailed)
  {
    gDialog.FinalStatusMessage.value = GetString("TransferCompleted");
    if (gFileNotFound && gTotalFileCount - gSucceededCount)
    {
      // Show number of files that failed to upload
      gStatusMessage = GetString("FailedFileMsg");
      gStatusMessage = gStatusMessage.replace(/%x%/,
                                          gTotalFileCount - gSucceededCount);
      gStatusMessage = gStatusMessage.replace(/%total%/, gTotalFileCount);

      SetGlobalStatusMessage(gStatusMessage);
    }
  }

  if (gTimeout > 0)
    // Leave window open a minimum amount of time 
    gTimerID = setTimeout(CloseDialog, gTimeout);
  else
    CloseDialog();
}

function CloseDialog()
{
  SaveWindowLocation();
  PassBackParams();
  try {
    window.close();
  } catch (e) {}
}

function onClose()
{
  if (gTimerID)
  {
    clearTimeout(gTimerID);
    gTimerID = null;
  }

  if (!gFinished)
  {
    gTransfer.cancel();
  }
  SaveWindowLocation();
  PassBackParams();

  return true;
}


// called, if the user presses Cancel in the conflict resolve dialog
function onCancel()
{
  onClose();
  CloseDialog();
}

function SetWindowLocation()
{
}

function SaveWindowLocation()
{
}

function AllowDefaultButton()
{
  gDialog.Close.setAttribute("default","true");
  gAllowEnterKey = true;
}

function onEnterKey()
{
  if (gAllowEnterKey)
    return CloseDialog();

  return false;
}

function SetGlobalStatusMessage(message)
{
  alert(message); // XXX
  CloseDialog(); // XXX
  //SetFileStatusMessage(-1, message)
}

function SetFileStatusMessage(filei, message)
{
  if (gTransfer.files[filei].status == "failed")
  {
    document.getElementById("errors").hidden = false;
    window.sizeToContent();
  }
  return;//XXX
  // Status message is a child of <description> element
  //  so text can wrap to multiple lines if necessary
  var textNode = document.createTextNode(message);
  if (textNode)
    gDialog.StatusMessage.appendChild(textNode);

  /*
  if (gDialog.StatusMessage.firstChild)
  {
    gDialog.StatusMessage.firstChild.data = message;
  }
  else
  {
    var textNode = document.createTextNode(message);
    if (textNode)
      gDialog.StatusMessage.appendChild(textNode);
  }
  */
  window.sizeToContent();
}

//XXX hack
function showErrors()
{
  var text = "";
  for (var i = 0, l = gTransfer.files.length; i < l; i++)
  {
    var file = gTransfer.files[i];
    text += ErrorMessageForFile(file) + "\n";
  }
  alert(text);
}
