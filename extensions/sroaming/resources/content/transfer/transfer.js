/*-*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org Session Roaming code.
 *
 * The Initial Developer of the Original Code is 
 *       Ben Bucksch <http://www.bucksch.org>
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *       Netscape Editor team
 *       ManyOne <http://www.manyone.net>
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
 * the terms of any one of the NPL, the GPL or the LGPL.
 
 * ***** END LICENSE BLOCK ***** */

const nsIChannel = Components.interfaces.nsIChannel;

  /*
    Passes parameters for transfer.

    @param download  Bool  download or upload
    @param serial  Bool  true: transfer one file after the other
                         false: all files at once (parallel)
    @param localDir  String  file:-URL of the local dir,
                               where the profile files are stored
    @param remoteDir  String   URL of the dir on the server,
                               where the profile files are stored
    @param password   String   Password for server in cleartext
    @param savepw     Bool     Should password be stored by def. when asked?
    @param files  Array of Objects with properties
                               filename  String  path relative to
                                                 localDir/remoteDir, plus
                                                 filename (incl. possible
                                                 extension)
                               mimetype  String  undefined, if unknown
                               size      int     in bytes.
                                                 undefined, if unknown
    @param progessCallback  function(file)  to be called when the progress
                               of a file transfer changed. can be used to e.g.
                               update the UI. parameter file is an entry in the
                               files array of this object, see makeFileVar()
                               and [this.]dump().
    @param finishedCallback  function(bool success)  to be called when
                               we're done. |success|, if it seems that
                               all files have been transferred successfully
                               and completely.
   */
function Transfer(download, serial,
                  localDir, remoteDir,
                  password, savepw,
                  files,
                  finishedCallback, progressCallback)
{
  this.download = download;
  this.serial = serial;
  this.localDir = localDir;
  this.remoteDir = remoteDir;

  var ioserv = GetIOService();
  var uri = ioserv.newURI(remoteDir, null, null);
  this.username = uri.username;
  this.password = password;
  this.savePassword = savepw; //savepw is when the user checked the checkbox
  this.saveLogin = false;     //savelogin is when we will write to disk
  this.sitename = uri.host;

  this.files = new Array();
  for (var i = 0, l = files.length; i < l; i++)
  {
    this.files[i] = this.makeFileVar(files[i].filename,
                                     files[i].mimetype,
                                     files[i].size);
  }

  this.progressSizeAll = 0;
  for (var i = 0, l = this.files.length; i < l; i++)
    this.progressSizeAll += this.files[i].size;
                    // We fixed up this.files[].size, so files != this.files
  this.progressSizeCompletedFiles = 0;
    /* Cumulated size of the files which have been *completely* (not partially)
       transferred. Used to speed up processing of incremental updates. */

  if (finishedCallback == null)
    finishedCallback = undefined;
  if (progressCallback == null)
    progressCallback = undefined;
  this.finishedCallback = finishedCallback;
  this.progressCallback = progressCallback;
  this.dump();
}
Transfer.prototype =
{
  /* factory function.
     returns a new object to be added to the this.files[] array.
  */
  makeFileVar : function(filename, mimetype, size) // private, static
  {
    // sanitize input
    if (mimetype == undefined || mimetype == "")
      mimetype = "application/octet-stream";
    if (size == undefined)
      size = 0;

    // for explanations, see this.dump()
    var file = new Object();
    file.filename = filename;
    file.mimetype = mimetype;
    file.size = size;
    ddump("got file " + filename + " with mimetype " + mimetype);
    file.status = "pending";
    file.statusCode = 0;
    file.progress = 0.0;
    file.channel = null;
    file.progressListener = null;
    return file;
  },

  /* Called when all files finished
     @param  success  bool  no file failed
  */
  done : function(success)
  {
    ddump("Done(" + (success ? "without" : "with") + " errors)");

    if (this.finishedCallback)
      this.finishedCallback(success);

    /* The close dialog stuff (when all files are done successfully) is UI and
       thus in sroamingProgress.js, called from SetProgressStatus(), which
       should already be called. */
  },

  /* Initiate transfer of files
     Note: when this function returns, the transfer is most likely *not*
     finished yet.
     @throws "transfer failed" (from transferFile)
   */
  transfer : function()
  {
    if (this.serial)
    {
      this.nextFile();
    }
    else
    {
      for (var i = 0, l = this.files.length; i < l; i++)
        this.transferFile(i);
    }
  },

  /* In serial mode, start the download of the next file, because all
     previous downloads are finished.
     @return  1: there are still files busy (you did something wrong)
              2: transfer of next file started
              3: all files were already done, nothing left to do.
     @throws "transfer failed" (from transferFile)
   */
  nextFile : function()
  {
    // check that there are no downloads in progress
    for (var i = 0, l = this.files.length; i < l; i++)
    {
      if (this.files[i].status == "busy")
      {
        dumpError("Programming error: nextFile() called, although there is"
                  + "still a download in progress.");
        this.dump();
        return 1;
      }
    }

    // find the next pending one
    for (var i = 0, l = this.files.length; i < l; i++)
    {
      if (this.files[i].status == "pending")
      {
        this.transferFile(i);
        return 2;
      }
    }

    return 3;
  },

  /* Transfer that file.
     Implements the actual upload/download.
     Independent of serial/parallel (caller is responsible for that).

     @param  id  index of this.files array of the file to be transferred
     @throws "transfer failed"
   */
  transferFile : function(id)
  {
    var file = this.files[id];
    ddump("TransferFile(" + id + ")");

    try
    {
      var ioserv = GetIOService();
      var baseURL = ioserv.newURI(this.remoteDir, null, null);
      baseURL.password = this.password;
      var localURL = ioserv.newURI(this.localDir, null, null);
      var channel = ioserv.newChannel(file.filename, null, baseURL);
      this.files[id].channel = channel;
      var progress = new SRoamingProgressListener(this, id);
      this.files[id].progressListener = progress;
      channel.notificationCallbacks = progress;
      ddumpCont("Trying to "+ (this.download ? "download from" : "upload to"));
      ddumpCont(" remote URL <" + channel.URI.spec + "> ");

      if (this.download)
      {
        var fos = Components
                   .classes["@mozilla.org/network/file-output-stream;1"]
                   .createInstance(Components.interfaces.nsIFileOutputStream);
        var ssl = Components
                   .classes["@mozilla.org/network/simple-stream-listener;1"]
                   .createInstance(Components.interfaces
                                   .nsISimpleStreamListener);
        var lf = ioserv.newURI(file.filename, null, localURL)
                       .QueryInterface(Components.interfaces.nsIFileURL)
                       .file.clone();
        ddump("to local file " + lf.path);
        try {
          fos.init(lf, -1, -1, 0);//odd params from NS_NewLocalFileOutputStream
        } catch(e) {
          var dir = lf.parent;
          if (e.name == "NS_ERROR_FILE_NOT_FOUND"
               // we get this error, if the directory does no exist yet locally
              && dir && !dir.exists())
          {
            ddumpCont("Creating dir " + dir.path + " ... ");
            dir.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
                          // also creates all higher-level dirs, if necessary
            ddump("done (hopefully).");
            fos.init(lf, -1, -1, 0); // try again
          }
          else
            throw e; // pass on all other errors
        }
        ssl.init(fos, progress);
        channel.asyncOpen(ssl, null);
      }
      else
      {
        var uploadChannel = channel.QueryInterface(
                                      Components.interfaces.nsIUploadChannel);
        var lfURL = ioserv.newURI(file.filename, null, localURL);
        ddump("from local file <" + lfURL.spec + ">");

        // way 1
        /*
        var lf = lfURL.QueryInterface(Components.interfaces.nsIFileURL)
                 .file.clone();
        uploadChannel.setUploadFile(lf, file.mimetype, -1);
        */

        // way 2  -- this is the way brade used. why didn't she use way 1?
        var fileChannel = ioserv.newChannelFromURI(lfURL);
        var is = fileChannel.open(); // maybe async? WFM.
        var bis = Components
                   .classes["@mozilla.org/network/buffered-input-stream;1"]
                   .createInstance(Components.interfaces
                                   .nsIBufferedInputStream);
        bis.init(is, 32768);
        uploadChannel.setUploadStream(bis, file.mimetype, -1);
        /* I *must* use a mimetype here. Passing null will cause the
           HTTP protocol implementation to use POST instead of PUT. */

        channel.asyncOpen(progress, null);
      }
    } catch(e) {
      dumpError("Transfer failed:\n" + e);
      throw "transfer failed";
    }
  },

  // called when an individual file transfer completed or failed
  fileFinished : function(filei)
  {
    ddump("file " + filei + " finished");
    if (this.serial)
      this.nextFile();

    // check, if we're all done
    var done = true;
    var failed = false;
    for (var i = 0, l = this.files.length; i < l; i++)
    {
      if (this.files[i].status == "failed")
        failed = true; // |done| stays true
      else if (this.files[i].status != "done") // failed already tested above
        done = false;
    }
    if (done)
      this.done(!failed);
  },

  // user wishes transfer to stop
  cancel : function()
  {
    ddump("Canceling transfer");

    // network stuff
    for (var i = 0, l = this.files.length; i < l; i++)
    {
      var channel = this.files[i].channel;
      if (channel && channel.isPending && this.files[i].status != "done")
      {
        channel.cancel(kErrorAbort);
        this.files[i].progressListener.setStatus("failed", kErrorAbort);
        /* the above line sets all files in the UI to failed, but that does
           not close the dialog. */
      }
    }
    if(this.finishedCallback)
      this.finishedCallback(false); // called twice, because of done()?
  },


  // Progress

  // functions dealing with tracking progress
  // not sure, if that should be at the progress listener

  progress : 0.0, // Gives the overall transfer progress of all files (0..1)
  progressCalculate : true, // if false, don't update this.progress

  /* The progress listener calls this function to notify it that the
     transfer progress of a certain file changed.
     This function will update the file's progress this.files[].progress
     and the overall progress this.progress.
     It will also call the creator's callbacks.

     @param filei  int  index of the file whose progress changed
     @param aProgress  float 0..1  progress of that file
  */
  progressChanged : function(filei, aProgress)
  {
    if (!this.progressCalculate)
      return;

    ddump("Transfer.progressChanged(" + filei + ", " + aProgress + ")");

    this.files[filei].progress = aProgress;

    if (this.progressSizeAll == 0) // undetermined mode
      return;

    var file = this.files[filei]; // don't move up

    // avoid too many updates
    if (file.progress - file.progressPrevious < 0.01)
      return;
    else
      this.files[filei].progressPrevious = file.progress;


    // calculate progress
    var progressSize = 0;
    if (file.status == "done")
    {
      ddump(" file completed mode");
      ddump(" progressSizeAll: " + this.progressSizeAll);
      ddumpCont(" progressSizeCompletedFiles before: ");
      ddump(this.progressSizeCompletedFiles);

      this.progressSizeCompletedFiles = 0;
      for (var i = 0, l = this.files.length; i < l; i++)
      {
        var cur = this.files[i];
        if (cur.status == "done")
          this.progressSizeCompletedFiles += cur.size;
      }
      progressSize = this.progressSizeCompletedFiles;

      ddumpCont(" progressSizeCompletedFiles now: ");
      ddump(this.progressSizeCompletedFiles);
      ddump(" progressSize: " + progressSize);
    }
    else
    {
      /* It's probably just an update during the file download -
         that happens often, so performance matters. */
      if (this.serial)
      {
        ddump(" update serial mode");
        ddump(" progressSizeAll: " + this.progressSizeAll);
        ddump(" progressSizeCompletedFiles: "+this.progressSizeCompletedFiles);
        ddump(" files[filei].size: " + file.size);
        ddump(" files[filei].progress: " + file.progress);

        progressSize = this.progressSizeCompletedFiles
                       + file.size * file.progress;

        ddump(" progressSize: " + progressSize);
      }
      else
      {
        ddump(" update parallel mode");
        ddump(" progressSizeCompletedFiles: "+this.progressSizeCompletedFiles);

        progressSize = this.progressSizeCompletedFiles;
        for (var i = 0, l = this.files.length; i < l; i++)
        {
          var cur = this.files[i];
          if (cur.status == "busy")
            progressSize += cur.size * cur.progress;
        }

        ddump(" progressSize: " + progressSize);
      }
    }

    this.progress = progressSize / this.progressSizeAll;
       // I protected at the very beginning against this.progressSizeAll == 0


    if(this.progressCallback)
      this.progressCallback(filei);
  },

  
  // functions dealing with conflict-(resolution), i.e. ensuring that
  // we don't accidently overwrite newer files

  /* Queries the last-modification-time and size of the roaming files,
     from the OS and writes them all into a special local file.
     This file will also be uploaded and later be used to compare the ...
  */
  saveFileStats : function()
  {
  },


  // Pass back info to dialog/c++.

  /* returns int, really enum:
    0 = do nothing, ignore all password/login changes
    1 = Save password, set SavePW pref (user checked the box in the prompt)
    2 = Set SavePassword pref to false and ignore Password/Username retval
  */
  getSaveLogin : function()
  {
  	if (this.saveLogin)
  	  return this.savePassword ? 1 : 2;
  	else
  	  return 0;
  },

  getPassword : function()
  {
  	if (this.getSaveLogin() == 1)
      return this.password;
    else
      return null;
  },

  getUsername : function()
  {
  	if (this.getSaveLogin() == 1)
      return this.username;
    else
      return null;
  },


  // utility/debug

  /*
    Prints to the console the contents of this object.
   */
  dump : function()
  {
    ddump("Transfer object:");
    ddump(" download: " + this.download);
        // bool, see constructor
    ddump(" serial: " + this.serial);
        // bool, see constructor
    ddump(" localDir: " + this.localDir);
        // String, see constructor
    ddump(" remoteDir: " + this.remoteDir);
        // String, see constructor
    ddump(" password: " + this.password);
        // String, see constructor
    ddump(" savePassword: " + this.savePassword);
        // bool, see constructor
    ddump(" sitename: " + this.sitename);
        // String, to be displayed to the user identifying the remote server

    ddump(" progress: "+(this.progressCalculate ? this.progress : "disabled"));
        // float, 0..1, how much of the files (measured by filesizes) has
        // already been transfered
    ddump(" progressSizeAll: " + this.progressSizeAll);
        // int, in bytes, cumulated filesizes as reported by the creator

    ddump(" Files: (" + this.files.length + " files)");
    for (var i = 0, l = this.files.length; i < l; i++)
    {
      var file = this.files[i];
      ddump("  File " + i + ":");
      ddump("   filename: " + file.filename);
          // String: relative pathname from profile root
      ddump("   mimetype: " + file.mimetype);
          // String. might be undefined.
      ddump("   size: " + file.size);
          // int, in bytes. might be 0 for undefined.
      ddump("   status: " + file.status);
          // String: "pending" (not started), "busy", "done", "failed"
      ddumpCont("   statusCode: " + file.statusCode + " (");
      ddump(NameForStatusCode(file.statusCode) + ")");
          // int: nsresult error code that we got from Necko
      ddump("   httpResponse: " + file.httpResponse);
          // int: HTTP response code that we got from the server
      ddump("   statusText: " + file.statusText);
          // String: Text corresponding to httpResponse
          // Text corresponding to statusCode can be gotten from ...utility.js
      ddump("   progress: " + file.progress);
          // float: 0 (nothing) .. 1 (complete)
    }
    //ddump(" finished callback: " + this.finishedCallback);
    //ddump(" progress callback: " + this.progressCallback);
    ddump(" finished callback: " + (this.finishedCallback ? "exists":"none"));
    ddump(" progress callback: " + (this.progressCallback ? "exists":"none"));
  }
}






/* Use one object per file to be downloaded.
   @param filei  integer  index of file in transfer.files
   @param transfer  Transfer  the context
*/
function SRoamingProgressListener(transfer, filei)
{
  this.transfer = transfer;  // this creates a cyclic reference :-(
  this.filei = filei;
}
SRoamingProgressListener.prototype =
{


  // Progress

  // nsIRequestObserver

  onStartRequest : function(aRequest, aContext)
  {
    ddump("onStartRequest: " + aRequest.name);
    this.setStatus("busy");
    //this.transfer.files[this.filei].status = "busy";
    //if(this.transfer.progressCallback)
    //  this.transfer.progressCallback(this.filei);
  },

  onStopRequest : function(aRequest, aContext, aStatusCode)
  {
    ddump("onStopRequest:");
    ddump("  Request: " + aRequest.name);
    ddump("  StatusCode: " + NameForStatusCode(aStatusCode));

    if (aStatusCode == kNS_OK)
    {
      /* HTTP gives us NS_OK, although the request failed with an HTTP error
         code, so check for that */
      var channel = this.transfer.files[this.filei].channel;
      if (!channel || !channel.URI)
        this.setStatus("failed", kErrorUnexpected);
      else if (channel.URI.scheme == "http")
        this.privateHTTPResponse();
      else
        // let's hope that the other protocol impl. are saner
        this.setStatus("done", aStatusCode);
    }
    else
      this.privateStatusChange(aStatusCode);
  },

  // nsIProgressEventSink

  onStatus : function(aRequest, aContext, aStatusCode, aStatusArg)
  {
    ddump("onStatus:");
    ddump("  Request: " + aRequest.name);
    ddump("  StatusCode: " + NameForStatusCode(aStatusCode));
    ddump("  StatusArg: " + aStatusArg);

    this.privateStatusChange(aStatusCode);
  },

  onProgress : function(aRequest, aContext, aProgress, aProgressMax)
  {
    ddumpCont("onProgress: " + aRequest.name + ", ");
    ddump(aProgress + "/" + aProgressMax);

    if (aProgressMax > 0)  // Necko sometimes sends crap like 397/0
    {
      this.transfer.progressChanged(this.filei, aProgress / aProgressMax);
    }
  },

  /* Use this function to interpret Mozilla status/error codes and
     update |this| appropriately.
     Don't use it for NS_OK - what this means depends on the situation and
     thus can't be interpreted here.
     @param aStatusCode  Mozilla error code
  */
  privateStatusChange : function(aStatusCode)
  {
    var status;
    if (aStatusCode == kNS_OK)
      // don't know what this means, only the caller knows what succeeded
      return;
    else if (aStatusCode == kStatusReadFrom)
      status = "busy";
    else if (aStatusCode == kStatusRecievingFrom)
      status = "busy";
    else if (aStatusCode == kStatusSendingTo)
      status = "busy";
    else if (aStatusCode == kErrorBindingRedirected)
      status = "busy";
    else if (aStatusCode == kErrorBindingRetargeted)
      status = "busy";
    else if (aStatusCode == kStatusResolvingHost)
      status = "busy";
    else if (aStatusCode == kStatusConnectedTo)
      status = "busy";
    else if (aStatusCode == kStatusConnectingTo)
      status = "busy";
    else if (aStatusCode == kStatusBeginFTPTransaction)
      status = "busy";
    else if (aStatusCode == kStatusEndFTPTransaction)
      status = "done";
    else if (aStatusCode == kNetReset)
      ; // correct?
    else
      status = "failed";

    this.setStatus(status, aStatusCode);
  },

  /* Use this function to get and interpret HTTP status/error codes and
     update |this| appropriately.
  */
  privateHTTPResponse : function()
  {
    ddumpCont("privateHTTPResponse ");

    // Get HTTP response code
    var responseCode;
    try
    {
      var httpchannel = this.transfer.files[this.filei].channel
                        .QueryInterface(Components.interfaces.nsIHttpChannel);
      responseCode = httpchannel.responseStatus;
      this.transfer.files[this.filei].httpResponse = responseCode;
      this.transfer.files[this.filei].statusText =
                                               httpchannel.responseStatusText;
      ddump(responseCode);
    }
    catch(e)
    {
      ddump("  Error while trying to get HTTP response: " + e);
      this.setStatus("failed", kErrorNoInterface);
      return;
    }

    // Interpret HTTP response code
    var status;
    if (responseCode >= 100 && responseCode < 200)
      status = "busy";
    else if (responseCode >= 200 && responseCode < 300)
      status = "done";
    else if (responseCode >= 300 && responseCode < 400)
      status = "busy";
    else if (responseCode >= 400 && responseCode < 600)
      status = "failed";
    else // what?
      return;

    this.setStatus(status, kStatusHTTP);
  },

  /* Use this function to change this.status. Maybe you want to use
     this.privateStatusChange, though.

     @param aStatus  string  like this.status, i.e. "done", "pending" etc.
     @param aStatusCode  Mozilla error code
  */
  setStatus : function(aStatus, aStatusCode)
  {
    ddumpCont("file " + this.filei + " changed from ");
    ddumpCont(this.transfer.files[this.filei].status + " (");
    ddumpCont(NameForStatusCode(this.transfer.files[this.filei].statusCode));
    ddumpCont(") to " + aStatus + " (");
    var undef = aStatusCode == undefined;
    ddump((undef ? "no new status code" : NameForStatusCode(aStatusCode))+")");
    if (this.transfer.files[this.filei].statusCode == aStatusCode
        && this.transfer.files[this.filei].status == aStatus)
    {
      ddump("  (no changes)");
      return;
    }

    var was_failed = (this.transfer.files[this.filei].status == "failed");
    var was_done = (this.transfer.files[this.filei].status == "done");
    if (!was_failed)  /* prevent overwriting older errors with newer, bogus
                         OK status codes, or the real problem with
                         consequential problems. */
    {
      this.transfer.files[this.filei].status = aStatus;
      if (aStatusCode != undefined)
        this.transfer.files[this.filei].statusCode = aStatusCode;
      if(this.transfer.progressCallback)
        this.transfer.progressCallback(this.filei);
    }
    if (
        !(was_done || was_failed)
        && (aStatus == "done" || aStatus == "failed")
       )  // file just completed or failed
     this.transfer.fileFinished(this.filei);
  },


  // Dummies

  // nsStreamListener

  onDataAvailable : function(aRequest, aContext,
                             anInputStream, anOffset, aCount)
  {
    /* dummy, because it's only the down stream during an upload
       and during download, we use the tee, which copies to the outstream. */
  },

  // nsIFTPEventSink, nsIHTTPEventSink

  OnFTPControlLog : function(aServer, aMsg)
  {
    // dummy
  },

  OnRedirect : function(anHTTPChannel, aNewChannel)
  {
    // dummy
  },


  // Prompts

  // nsIPrompt
  alert : function(dlgTitle, text)
  {
    ddump("alert");
    if (!title)
      title = GetString("Alert");
    GetPromptService().alert(window, title, message);
  },
  alertCheck : function(dlgTitle, text, checkBoxLabel, checkObj)
  {
    ddump("alertCheck");
    this.alert(dlgTitle, text);
  },
  confirm : function(dlgTitle, text)
  {
    ddump("confirm");
    return this.confirmCheck(dlgTitle, text, null, null);
  },
  confirmCheck : function(dlgTitle, text, checkBoxLabel, checkObj)
  {
    ddump("confirmCheck");
    var promptServ = GetPromptService();
    if (!promptServ)
      return;

    promptServ.confirmEx(window, dlgTitle, text,
                         nsIPromptService.STD_OK_CANCEL_BUTTONS,
                         "", "", "", checkBoxLabel, checkObj);
  },
  confirmEx : function(dlgTitle, text, btnFlags,
                       btn0Title, btn1Title, btn2Title,
                       checkBoxLabel, checkVal)
  {
    ddump("confirmEx");
    var promptServ = GetPromptService();
    if (!promptServ)
      return 0;

    return promptServ.confirmEx(window, dlgTitle, text, btnFlags,
                        btn0Title, btn1Title, btn2Title,
                        checkBoxLabel, checkVal);
  },
  select : function(dlgTitle, text, count, selectList, outSelection)
  {
    ddump("select");

    var promptServ = GetPromptService();
    if (!promptServ)
      return false;

    return promptServ.select(window, dlgTitle, text, count,
                             selectList, outSelection);
  },
  prompt : function(dlgTitle, label, /*inout*/ inputvalueObj,
                    checkBoxLabel, /*inout*/ checkObj)
  {
    ddump("nsIPrompt::prompt");
    return this.privatePrompt(dlgTitle, label, null, inputvalueObj,
                              checkBoxLabel, checkObj);
  },
  /* Warning: these |promptPassword| and |promptUsernameAndPassword|
     from nsIPrompt will never be invoked. If they are being called, the
     nsIAuthPrompt versions will be invoked, with wrong arguments.
     This is because we're overloading the functions with the same number
     of parameters and JS can't distinguish between different parameter types.
     This is a bug copied from Editor's Publishing feature; the bug exists
     there as well. */
  promptPassword : function(dlgTitle, label, /*inout*/ pwObj,
                            checkBoxLabel, /*inout*/ savePWObj)
  {
    ddump("nsIPrompt::promptPassword:");
    ddump("  pwObj: " + pwObj.value);
    ddump("  checkBoxLabel: " + checkBoxLabel);
    ddump("  savePWObj: " + savePWObj.value + " (ignored)");

    var ret = this.privatePromptPassword(dlgTitle, label, null,
                                         pwObj, checkBoxLabel);

    savePWObj.value = this.transfer.savePassword; // out param
    return ret;
  },
  promptUsernameAndPassword : function(dlgTitle, label,
                                       /*inout*/ userObj, /*inout*/ pwObj,
                                       savePWLabel, /*inout*/ savePWObj)
  {
    ddump("nsIPrompt::promptUsernameAndPassword:");
    ddump("  userObj: " + userObj.value);
    ddump("  pwObj: " + pwObj.value);
    ddump("  savePWLabel: " + savePWLabel);
    ddump("  savePWObj: " + savePWObj.value + " (ignored)");

    var ret = this.privatePromptUsernameAndPassword(dlgTitle, label, null,
                                                    userObj, pwObj,
                                                    savePWLabel);

    savePWObj.value = this.transfer.savePassword; // out param
    return ret;
  },

  // nsIAuthPrompt

  prompt : function(dlgTitle, label, pwrealm, /*in*/ savePW, defaultText,
                    /*out*/ resultObj)
  {
    ddump("nsIAuthPrompt::prompt");
    var savePWObj = {value:this.transfer.savePassword};
                    // I know it better than the caller.
    var inputvalueObj = {value:defaultText};
    var ret = this.privatePrompt(dlgTitle, label, pwrealm,
                                 inputvalueObj, null, savePWObj);
    resultObj.value = inputvalueObj.value;
    return ret;
  },
  promptPassword : function(dlgTitle, label, pwrealm, /*in*/ savePW,
                            /*out*/ pwObj)
  {
    ddump("nsIAuthPrompt::promptPassword");
    ddump("  pwrealm: " + pwrealm);
    ddump("  savePW: " + savePW + " (ignored)");
    ddump("  pwObj: " + pwObj.value);

    return this.privatePromptPassword(dlgTitle, label, pwrealm,
                                      pwObj, null);
  },
  promptUsernameAndPassword : function(dlgTitle, label, pwrealm, /*in*/savePW,
                                       /*out*/ userObj, /*out*/ pwObj)
  {
    ddump("nsIAuthPrompt::promptUsernameAndPassword:");
    ddump("  pwrealm: " + pwrealm);
    ddump("  savePW: " + savePW + " (ignored)");
    ddump("  userObj: " + userObj.value);
    ddump("  pwObj: " + pwObj.value);

    return this.privatePromptUsernameAndPassword(dlgTitle, label, pwrealm,
                                                 userObj, pwObj, null);
  },

  // helper functions for prompt stuff

  privatePrompt : function(dlgTitle, label, pwrealm, /*inout*/ inputvalueObj,
                           checkBoxLabel, /*inout*/ checkObj)
  {
    if (!dlgTitle)
      dlgTitle = GetString("PasswordTitle");
    if (!pwrealm)
      pwrealm = this.transfer.sitename;
    if (!label)
      label = this.makePWLabel(userObj.value, pwrealm);
    if (!checkBoxLabel)
      checkBoxLabel = GetString("SavePassword");

    var promptServ = GetPromptService();
    if (!promptServ)
       return false;

    var ret = promptServ.prompt(window, dlgTitle, label,
                                inputvalueObj, checkBoxLabel, checkObj);
    if (!ret)
      setTimeout(this.transfer.cancel, 0);
    return ret;
  },
  privatePromptPassword : function(dlgTitle, label, pwrealm,
                                   /*inout*/ pwObj, savePWLabel)
  {
    ddump("privatePromptPassword()");
    ddump("  pwObj.value: " + pwObj.value);
    ddump("  savePWLabel: " + savePWLabel);
    ddump("  label: " + label);
    ddump("  pwrealm: " + pwrealm);
    ddump("  this.transfer.savePassword: " + this.transfer.savePassword);
    ddump("  this.transfer.username: " + this.transfer.username);

    if (!dlgTitle)
      dlgTitle = GetString("PasswordTitle");
    if (!pwrealm)
      pwrealm = this.transfer.sitename;
    if (!label)
      label = this.makePWLabel(userObj.value, pwrealm);
    if (!savePWLabel)
      savePWLabel = GetString("SavePassword");
    var savePWObj = {value: this.transfer.savePassword};

    ddump("  pwObj.value: " + pwObj.value);
    ddump("  savePWLabel: " + savePWLabel);
    ddump("  label: " + label);
    ddump("  pwrealm: " + pwrealm);

    var promptServ = GetPromptService();
    if (!promptServ)
      return false;

    var ret = false;
    try {
      ret = promptServ.promptPassword(window, dlgTitle, label,
                                      pwObj, savePWLabel, savePWObj);

      if (!ret)
        setTimeout(this.transfer.cancel, 0);
      else
        this.updateUsernamePasswordFromPrompt(this.transfer.username,
                                              pwObj.value, savePWObj.value);
    } catch(e) {}

    return ret;
  },
  privatePromptUsernameAndPassword : function(dlgTitle, label, pwrealm,
                                           /*inout*/ userObj, /*inout*/ pwObj,
                                           savePWLabel)
  {
    // HTTP prompts us twice even if user Cancels from 1st attempt!

    ddump("privatePromptUsernameAndPassword()");
    ddump("  pwObj.value: " + pwObj.value);
    ddump("  userObj.value: " + userObj.value);
    ddump("  savePWLabel: " + savePWLabel);
    ddump("  label: " + label);
    ddump("  pwrealm: " + pwrealm);
    ddump("  this.transfer.savePassword: " + this.transfer.savePassword);
    ddump("  this.transfer.username: " + this.transfer.username);

    if (!dlgTitle)
      dlgTitle = GetString("PasswordTitle");
    if (!pwrealm)
      pwrealm = this.transfer.sitename;
    if (!label)
      label = this.makePWLabel(userObj.value, pwrealm);
    if (!savePWLabel)
      savePWLabel = GetString("SavePassword");
    if (!userObj.value)
      // HTTP PUT uses this dialog if either username or password is bad,
      // so prefill username with the previous value
      userObj.value = this.transfer.username;
    var savePWObj = {value: this.transfer.savePassword};

    ddump("  savePWObj.value: " + savePWObj.value);
    ddump("  userObj.value: " + userObj.value);
    ddump("  savePWLabel: " + savePWLabel);
    ddump("  label: " + label);
    ddump("  pwrealm: " + pwrealm);

    /* I had the idea to put up the password-only dialog, if we have a
       username. But if the user didn't enter a username in the prefs,
       it first all works correctly, the username+password dialog comes up.
       The problem starts when the user entered the wrong username -
       he won't have a chance to change it anymore. Not sure, if that is bad.
       So, the code for that is below.
       Update: I decided that the user has to enter a username in any case,
       and he can't change it during login. At least not in this case. */

    if (userObj.value && userObj.value != "")
    {
      // |label| here says "enter username and password ...", so we need
      // to build our own.
      label = this.makePWLabel(userObj.value, pwrealm);
      return this.privatePromptPassword(dlgTitle, label, pwrealm,
                                        pwObj, null);
    }

    // We should get here only, if we got an empty username pref from the reg.
    ddump("  asking for username as well");

    var ret = false;
    try {
      var promptServ = GetPromptService();
      if (!promptServ)
        return false;

      ret = promptServ.promptUsernameAndPassword(window,
                                        dlgTitle, label, userObj, pwObj, 
                                        savePWLabel, savePWObj);

      if (!ret)
        setTimeout(this.transfer.cancel, 0);
      else
        this.updateUsernamePasswordFromPrompt(userObj.value, pwObj.value,
                                              savePWObj.value);
    } catch (e) {}
    ddump("  done. result:");
    ddump("  userObj.value: " + userObj.value);
    ddump("  pwObj.value: " + pwObj.value);
    ddump("  savePWObj.value: " + savePWObj.value);
    return ret;
  },
  // Update any data that the user supplied in a prompt dialog
  updateUsernamePasswordFromPrompt : function(username, password,savePassword)
  {
    // Set flag to save login data after transfering, if it changed in
    // dialog and the "SavePassword" checkbox was checked
    this.transfer.saveLogin = (username != this.transfer.username ||
                               password != this.transfer.password)
                              && savePassword;

    ddump("  username: " + username);
    ddump("  password: " + password);
    ddump("  savePW: " + savePassword);
    ddump("  saveLogin: " + this.transfer.saveLogin);

    this.transfer.username = username;
    this.transfer.password = password;
    this.transfer.savePassword = savePassword;
  },
  makePWLabel : function (user, realm)
  {
  	if (!user)
      user = "";
    var label = GetString("EnterPasswordForUserAt");
    label = label.replace(/%username%/, user);
    label = label.replace(/%realm%/, realm);
    return label;
  },


  // XPCOM

  // nsISupports

  QueryInterface : function(aIID)
  {
    //ddump("QI: " + aIID.toString() + "\n");
    if (aIID.equals(Components.interfaces.nsIProgressEventSink)
     || aIID.equals(Components.interfaces.nsIRequestObserver)
     || aIID.equals(Components.interfaces.nsIStreamListener)
     || aIID.equals(Components.interfaces.nsISupports)
     || aIID.equals(Components.interfaces.nsISupportsWeakReference)
     || aIID.equals(Components.interfaces.nsIPrompt)
     || aIID.equals(Components.interfaces.nsIAuthPrompt)
     || aIID.equals(Components.interfaces.nsIFTPEventSink)
     || aIID.equals(Components.interfaces.nsIHttpEventSink)
     || aIID.equals(Components.interfaces.nsIInterfaceRequestor))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },

  // nsIInterfaceRequestor

  getInterface : function(aIID)
  {
    return this.QueryInterface(aIID);
  }
}



function GetPromptService()
{
  var promptService = null;
  try {
    promptService = Components
                     .classes["@mozilla.org/embedcomp/prompt-service;1"]
                     .getService(Components.interfaces.nsIPromptService);
  } catch(e) {}
  return promptService;
}


function GetIOService()
{
  var ioService;
  try {
    ioService = Components.classes["@mozilla.org/network/io-service;1"]
                 .getService(Components.interfaces.nsIIOService);
  } catch(e) {}
  return ioService;
}


/* Creates a new nsIFile instance from an URI.
   Currently unused!
 @param  uristring  a (possibly relative) file: URI as string
 @param  baseuri    if uristring is a relative URI, baseuri is a file: URI
                    to use as base. If baseuri is |null|, uristring is
                    assumed to be an absolute URI.
 @return  a fresh nsIFile for ya ;-P
 @exception  may throw a few ;-)
*/
function GetFileFromURI(uristring, baseuri)
{
  var ioserv = GetIOService();
  var ifile = Components.classes["@mozilla.org/file/local;1"]
               .createInstance(Components.interfaces.nsIFile);
  if (baseuri != null)
    uristring = ioserv.resolveRelativePath(uristring, baseuri);
  ioserv.initFileFromURLSpec(ifile, urlstring);
  return ifile;
}
