/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla Roaming code.
 *
 * The Initial Developer of the Original Code is (XXX am I?) 
 *       Ben Bucksch <http://www.bucksch.org>
 *       of Beonex <http://www.beonex.com>
 * Portions created by ManyOne are Copyright (C) 2003 ManyOne.
 * All Rights Reserved.
 * 
 * Contributor(s): 
 *       ManyOne <http://www.manyone.net>
 */

/* Make sure that we don't accidently overwrite files with newer
   or different changes. It will open a conflictResolve.xul dialog,
   if necessary. */

  /* First, we transfer the "listing" file from the server. This contains
     stats about the files on the server,
     like filenames, their last modification date and their filesizes.
     It is true that the same information could be obtained via the HTTP
     protocol (e.g. HTTP HEAD), but that would
     - be a lot harder to skip retrieval of already downloaded files,
       if we make only one connection per file,
       because we'd have to use modification time sent in the header right
       before the data and stop the transfer at the right time
     - be protocol-specific in the Mozilla implementation, unless we
       fix bug 186016 for all protocols we want to support
     - depend on the user's realtime clock to be correct
     The file format is XML, see example.

     Possible problems/conflicts:
     - Download
       - listing file transfer from server fails
       - listing from server is older than listing-uploaded
         (I don't think this can actually happen in practice, unless the
         clock is screwed)
       - listing-uploaded doesn't match profile files
         (incl. last download failed fatally)
       - download of individual files fails
     - Upload
       - listing file transfer from server fails
       - listing file from server is newer than listing-downloaded
         (other instance updated server while we were running or
         the last download failed)
       - listing file from server is newer than listing-uploaded
         Already covered by previous case / during download
       - listing from server is older than listing-downloaded
         (I don't think this can actually happen in practice, unless the
         clock is screwed)
       - upload of some files fails
       - upload fails fatally (crash etc.)
       - Mozilla crashed/term. before it had a chance to even start the upload

     Checks/Steps:
     - Download
       1. Transfer listing file from server to listing.xml
       2. Compare compare listing uploaded with listing.xml from server.
          - if they match, then we already have the newest version locally and
            don't need to download and don't have a problem/conflict either,
            even if the local files don't match listing.xml from server.
            We can just copy listing.xml to listing-downloaded (XXX?)
            and be done.
          - if they don't match, then either
            - another instanceuploaded after
              we ran the last time, which is not a problem in itself, but we
              need to download and make more checks for conflicts.
            - our last upload finished only partially, which we will consider
              a problem and a conflict XXX
            2a. Compare local files with listing-uploaded
                - if they don't match, we have a conflict (unless the listing
                  from the server matches listing-uploaded, but then we already
                  decided above that we don't have a problem)
       3. If there were conflicts, ask the user what to do
       4. Download files
       5. Check local files against listing.xml from server.
          XXX needed?
          - if they match (esp. filesizes), all went fine. Move listing.xml
            to listing-downloaded.
          - if they don't match, we have a severe problem. We just overwrote
            the local files (which are assumed to be good) with broken
            versions. Warn the user. Remove listing-downloaded, which
            will cause conflicts during the next upload, which allows the
            user to then prevent overwriting of the file on the server, which
            might still be OK. This won't help, if the upload failed although
            netwerk reported success, but I don't know how to protect against
            that apart from a temporary second copy of the files.
     - Upload
       1. Transfer listing file from server to listing.xml
       2. Compare listing-downloaded with listing.xml from server
          - if they match, our local version is the newest, so there is no
            conflict here.
          - if they don't match, then either
            - another instance uploaded while we ran. classic conflict.
              listing.xml won't match listing-uploaded in this case, so we
              can use the checks for the next case.
            - the last download failed.
            2a. Compare listing.xml from the server with listing-uploaded
                - If they match, then we don't have a problem here,
                  because we were the last ones who uploaded
                  and we probably have the same info still in the local files.
                - if the don't match, then another instance uploaded after
                  we last uploaded, and we didn't get that info yet, so we have
                  a conflict.
       3. If there were conflicts, ask the user what to do        
       4. Create listing file from local files
       5. Upload files incl. listing file
          - if there are any problems during upload, note them in listing-local
       6. mv listing to listing-uploaded
  */


 /*     upload:   created locally, uploaded together with files,
                  renamed to listing-uploaded
        download: downloaded from server, checked, download of files,
                  renamed to listing-server */
const kListingTransferFilename = "listing.xml";
      /* the server listing file we just downloaded or are uploading atm */
const kListingDownloadedFilename = "listing-downloadedfiles.xml";
              /* last version of files we downloaded from server. */
const kListingUploadedFilename = "listing-uploadedfiles.xml";
              /* last version of files we uploaded from this instance. */
const kListingMimetype = "text/xml";
const kProfileFilesMimetype = "application/octet-stream";
const kConflDlg = "chrome://sroaming/content/transfer/conflictResolve.xul";

var gLocalDir;

/* no object here, because this needs a lot of callbacks, and callbacks to
   object methods are tricky. */



/*
  Starts and performs the replication of needed files from the server
  to the local cache dir
  @param transfer  Transfer object  For the main files. We'll create a
                        second Transfer object, based on this one, for the
                        listing file.
  @param listingProgress  function(bool)  Callback to tell the dialog, if
                        the transfer of the listing file is in progress.
                        The dialog will probably display an indetermined
                        progressmeter, if the parameter is true.
  @result  nothing
 */
function checkAndTransfer(transfer)
{
  ddump("checking deps");

  gLocalDir = transfer.localDir;

  /* A few nested callbacks follow. Asyncronous processing... :-/
     At least the last one needs local vars here. I don't want to use
     global vars for that. */

  ddump("Step 1: Downloading listing file from server");
  var filesListing = new Array();
  filesListing[0] = new Object();
  filesListing[0].filename = kListingFileTransferFilename;
  filesListing[0].mimetype = kListingFileMimetype;
  filesListing[0].size = undefined;
  var listingTransfer = new Transfer(true, // download
                                     transfer.serial,
                                     transfer.localDir, transfer.remoteDir,
                                     transfer.password, transfer.savepw,
                                     filesListing,
                                     function()
  {
    ddump("loading remote listing file");
    var remoteListingDOM = document.implementation
                                   .createDocument("", "remoteListing", null);
    remoteListingDOM.load(gLocalDir + kListingTransferFilename);
    remoteListingDOM.addEventListener("load", function()
    {
      var remoteListing = readListingFile(remoteListingDOM);
      if (transfer.download)
        Download(transfer, remoteListing);
      else
        Upload(transfer, remoteListing);
    }, false);
  }, progressxxx); listingTransfer.transfer();
}


// download part of checkAndTransfer
function download(transfer, remoteListing)
{
  ddump("loading listing-uploaded file");
  var listingUploadedDOM = document.implementation
                                 .createDocument("", "listingUploaded", null);
  listingUploadedDOM.load(gLocalDir + kListingUploadedFilename);
  listingUploadedDOM.addEventListener("load", function()
  {
    var listingUploaded = readListingFile(listingUploadedDOM);

    ddump("Step 2: Comparing these listing files");
    var comparisonStep2 = compareFiles(transfer.files,
                                       remoteListing,
                                       listingUploaded) );
    if (comparisonStep2.mismatches.length == 0)
      // Match: files didn't change on the server since our last update
    {
      ddump("Step 3: Skipped, because no conflict found");
      // nothing to be done
    }
    else // Mismatch
    {
      /* Ignore list of matches, just consider all to mismatch. In normal
         operation (even expected error cases), either all should match or
         mismatch, so if a few match and a few mismatch, there is something
         goofy going on. XXX is this also true, if local files didn't exist
         yet? */
      ddump("Step 2a: Comparing local files with listing-uploaded");
      var localFiles = localFilesList(transfer.files);
      var comparisonStep2a = compareFiles(transfer.files,
                                          localFiles,
                                          listingUploaded);
      if (comparisonStep2a.mismatch.length == 0)
      {
        ddump("Step 3: Skipped, because no conflict found");
        // no conflict, but we need to download.
        // download all files, so no need to change transfer.files
      }
      else
      {
        ddump("Step 3: Asking user which version to use");
        var answer = conflictAsk(extractFiles(comparisonStep2a.mismatches,
                                              remoteListing),
                                 extractFiles(comparisonStep2a.mismatches,
                                              localFiles)      );
               /* for the mismatching files, pass the stats of the server
                  and local versions to the dialog */
        transfer.files = extractFiles(answer.server, transfer.files);
               /* only transfer those files that the user explicitly selected
                  for transfer, saving the rest from being overwritten. */
      }
    }

    ddump("Step 4: Downloading profile files");
    var originalMainFinishedCallback = transfer.finishedCallback;
    transfer.finishedCallback = function()
    {
      ddumpCont("Step 5: Check local files (incl. downloaded one) against ");
      ddump("server listing");
      //var localFiles = localFilesList(remoteListing);
      //checkFailedFiles(transfer);

      /* Cache files should now match the listing.xml from the server.
         Move listing.xml from server to listing-downloaded, to prevent
         overwriting during the next update and to record, which is the
         last version we got from the server, for future checks. */
      move(kListingTransferFilename, kListingDownloadedFilename);

      if (originalMainFinishedCallback)
        originalMainFinishedCallback();

      ddump("transfer done");
    }; transfer.transfer();
  }, false);
}


// upload part of checkAndTransfer
function upload(transfer, remoteListing)
{
  ddump("loading listing-downloaded file");
  var listingDownloadedDOM = document.implementation
                               .createDocument("", "listingDownloaded", null);
  listingDownloadedDOM.load(gLocalDir + kListingDownloadedFilename);
  listingDownloadedDOM.addEventListener("load", function()
  {
    var listingDownloaded = readListingFile(listingDownloadedDOM);
    var localFiles = localFilesList(transfer.files);

    ddump("Step 2: Comparing these listing files");
    var comparisonStep2 = compareFiles(transfer.files,
                                       remoteListing,
                                       listingDownloaded) );
    if (comparisonStep2.mismatches.length == 0)
      // Match: files didn't change on the server since our last update
    {
      ddump("Step 3: Skipped, because no conflict found");
    }
    else // Mismatch
    {
      /* Ignore list of matches, just consider all to mismatch. In normal
         operation (even expected error cases), either all should match or
         mismatch, so if a few match and a few mismatch, there is something
         goofy going on. XXX is this also true, if local files didn't exist
         yet? */
      ddump("Step 2a: Comparing listing from server with listing-uploaded");
      ddump("loading listing-uploaded file");
      var listingUploadedDOM = document.implementation
                                 .createDocument("", "listingUploaded", null);
      listingUploadedDOM.load(gLocalDir + kListingUploadedFilename);
      listingUploadedDOM.addEventListener("load", function()
      {
        var listingUploaded = readListingFile(listingUploadedDOM);

        var comparisonStep2a = compareFiles(transfer.files,
                                            remoteListing,
                                            listingUploaded);
        if (comparisonStep2a.mismatch.length == 0)
        {
          ddump("Step 3: Skipped, because no conflict found");
          /* no conflict (we were the last ones who uploaded),
             but we need to upload. */
          // upload all files, so no need to change transfer.files.
        }
        else
        {
          ddump("Step 3: Asking user which version to use");
          var answer = conflictAsk(extractFiles(comparisonStep2a.mismatches,
                                                remoteListing),
                                   extractFiles(comparisonStep2a.mismatches,
                                                localFiles)      );
                 /* for the mismatching files, pass the stats of the server
                    and local versions to the dialog */
          transfer.files = extractFiles(answer.server, transfer.files);
                 /* only transfer those files that the user explicitly selected
                    for transfer, saving the rest from being overwritten. */
        }
      }, false);
    }

    ddump("Step 4: Creating listing file from local files");
    createListingFile(localFiles, kListingTransferFilename);

    ddump("Step 5: Uploading profile files and listing file");
    // add listing file to the files to be uploaded
    var listingEntry = new Object();
    listingEntry.filename = kListingFileTransferFilename;
    listingEntry.mimetype = kListingFileMimetype;
    listingEntry.size = undefined;
    transfer.files.push(listingEntry);

    var originalMainFinishedCallback = transfer.finishedCallback;
    transfer.finishedCallback = function()
    {
      ddump("Step 6: mv listing to listing-uploaded");
      move(kListingTransferFilename, kListingUploadedFilename);

      if (originalMainFinishedCallback)
        originalMainFinishedCallback();

      ddump("transfer done");
    }; transfer.transfer();
  }, false);
}


/*
  Takes 3 lists of files. For each entry in filesList, it compares the
  corresponding entries in files1 and files2 and returns,
  which entries match and which don't.

  @param filesList  Array of Objects with property |filename| (similar to
                    result of readListingFile())
  @param files1  See result of readListingFile()
  @param files2  See result of readListingFile()
  @result  Object with propreties |matches| and |mismatches|, both of which
                    are new Arrays holding subsets of filesList.
*/
function compareFiles(filesList, files1, files2)
{
  ddump("comparing file lists");

  var result = new Object();
  result.matches = new Array();
  result.mismatches = new Array();

  // sanitize input
  if (!filesList)
    filesList = new Array();
  if (!files1)
    files1 = new Array();
  if (!files2)
    files2 = new Array();

  /* go through filesList and compare each entry with the corresponding ones
     in files1 and files2 */
  for (var i = 0, l = filesList.length; i < l; i++)
  {
    var filename = filesList[i].filename;

    // find corresponding entries
    var f1 = findEntry(files1, filename);
    var f2 = findEntry(files2, filename);

    // compare
    var matches = (f1 && f2
                   && f1.size == f2.size
                   && f1.lastModified == f2.lastModified);

    ddump(filename + (matches ? " matches" : " doesn't match"));
    if (matches)
      result.matches.push(filesList[i]);
    else
      result.mismatches.push(filesList[i]);
  }
  return result;
}


/*
  Utility function.
  For each entry in filesList, it returns the corresponding entry in files1.

  @param filesList  Array of Objects with property |filename| (similar to
                    result of readListingFile())
  @param files1  See result of readListingFile()
  @result  Like files1  (but with possibly less content)
*/
function extractFiles(filesList, files1)
{
  var result = new Array();

  // sanitize input
  if (!filesList)
    filesList = new Array();
  if (!files1)
    files1 = new Array();

  for (var i = 0, l = filesList.length; i < l; i++)
  {
    var f = findEntry(files1, filesList[i].filename);
    if (f)
      result.push(f);
  }
  return result;
}


/*
   Utility function.
   Returns filesList's entry which matches |filename|.

   Would probably be more efficient to use the filename as index/property,
   not to use arrays, but I don't completely rewrite for speed
   unless there is a real problem.

   @param filesList  see result of readListingFile()  an array to search in
   @param filename  string  the file to search for
   @return  one item in the filesList array  has matching filename
            null, if not found
*/
function findEntry(filesList, filename)
{
  var f;
  for (var i = 0, l = files1.length; i < l; i++)
  {
    if (files1[i].filename == filename)
      f = files1[i];
  }
  return f;
}


/*
  Takes the listing file from the server and reads it into the returned array

  @param listingFile  nsIDOMDocument?  listing file from the server,
                             already transferred and stored locally (??)
  @result  Array of Objects  list of files which have to be transferred.
                             Each object has 3 properties:
                             filename     path+filename relative to the
                                          local cache dir
                             mimetype     e.g. "text/plain" or
                                          "application/octet-stream"
                             size         in bytes
                             lastModifed  as Unix time
           nothing           in the case of an error
 */
function readListingFile(listingFile)
{
  ddump("will interpret listing file");
  if (!listingFile)
  {
    dumpError("got no listing file");
    return;
  }

  var files = new Array();
  try
  {
    var root = listingFile.childNodes; // document's children
    var listing; // <listing>'s children

    /* although the file has only one real root node, there are always
       spurious other nodes (#text etc.), which we have to skip/ignore */
    for (var i = 0, l = root.length; i < l; i++)
    {
      var curNode = root.item(i);
      if (curNode.nodeType == Node.ELEMENT_NODE
          && curNode.tagName == "listing")
        listing = curNode.childNodes;
    }
    if (listing == undefined)
    {
      dumpError("malformed listing file");
      return;
    }

    var curFile = 0;
    for (var i = 0, l = listing.length; i < l; i++)
                                                   // <file>s and <directory>s
    {
      var curNode = listing.item(i);
      if (curNode.nodeType == Node.ELEMENT_NODE && curNode.tagName == "file")
      {
        files[curFile] = new Object();
        files[curFile].filename = curNode.getAttribute("filename");
        files[curFile].mimetype = curNode.getAttribute("mimetype");
        files[curFile].size = parseInt(curNode.getAttribute("size"));
        files[curFile].lastModified =
                       parseInt(curNode.getAttribute("lastModifiedUnixtime"));
        curFile++;
      }
      // <directory>s not yet supported
      // ignore spurious nodes like #text etc.
    }
  }
  catch(e)
  {
    dumpError("Error while reading listing file: " + e);
  }

  dumpObject(files, "files");
  return files;
}

/* Creates an XML file on the filesystem from a files array.

   Example content:
<?xml version="1.0"?>
<listing>
  <file
    filename="index.html"
    mimetype="text/html"
    size="1950"
    lastModifiedUnixtime="1045508684000"
  />
</listing>

   @param files  see result of readListingFile()  data to be written to the 
                                                  file
   @param filename  string  filename of the to-be-created XML file, relative
                            to the local profile dir
   @result null
   @throws all errors (that can happen during file creation/writing)
 */
function createListingFile(files, filename)
{
  // create content as string
  var content = "<?xml version="1.0"?>\n<listing>\n";
                                     // content of the file to be written
  for (var i = 0, l = files.length; i < l; i++)
  {
    var f = files[i];
    content += "  <file\n"
               + "    filename=\"" + f.filename + "\"\n"
               + "    mimetype=\"" + f.mimetype + "\"\n"
               + "    size=\"" + f.size + "\"\n"
               + "    lastModifiedUnixtime=\"" + f.lastModified + "\"\n"
               + "  />\n";
  }
  content += "</listing>";

  // write string to file
  var lf = makeNSIFileFromRelFilename(filename);
  if (!lf)
    return;
  if (!lf.exists())
    lf.create(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0600); // needed?
  var fos = Components
            .classes["@mozilla.org/network/file-output-stream;1"]
            .createInstance(Components.interfaces.nsIFileOutputStream);
  var bos = Components
            .classes["@mozilla.org/network/buffered-output-stream;1"]
            .createInstance(Components.interfaces.nsIBufferedOutputStream);
  var ssl = Components
            .classes["@mozilla.org/network/simple-stream-listener;1"]
            .createInstance(Components.interfaces.nsISimpleStreamListener);
  fos.init(lf, -1, -1, 0); //odd params from NS_NewLocalFileOutputStream
  bos.init(fos, 32768);
  bos.write(content, content.length);
  bos.close(); // needed? throwing?
  fos.close(); // dito
}


/*
  Creates a files list (with stats from the filesystem) from the
  local profile files.
  @param  checkFiles  Array of Objects with property |filename|  List
                        of files to be checked
  @result  see result of readListingFile()  filesystem stats of the files
 */
function localFilesList(checkFiles)
{
  var result = new Array();

  for (var i = 0, l = checkFiles.length; i < l; i++)
  {
    var filename = checkFiles[i].filename;

    // create file entry for result array
    var f = new Object();
    f.filename = filename;

    try
    {
      var lf = makeNSIFileFromRelFilename(filename);
      if (!lf)
      {
        dumpError("couldn't create localfile instance for " + filename);
        // see below
        f.lastModified = 0;
        f.size = 0;
        f.mimetype = kProfileFilesMimetype;
      }
      if (lf.exists() && lf.isFile() && lf.isReadable() && lf.isWritable())
      {
        f.lastModified = lf.lastModifiedTime;
        f.size = lf.fileSize;
        f.mimetype = kProfileFilesMimetype;
      }
      else
      {
        // see below
        f.lastModified = 0;
        f.size = 0;
        f.mimetype = kProfileFilesMimetype;
      }
    }
    catch(e)
    {
      // XXX use error field? (see also above)
      f.lastModified = 0;
      f.size = 0;
      f.mimetype = kProfileFilesMimetype;
    }
    result.push(f);
  }
  return result;
}


/*
  We have a conflict (i.e. 2 versions of the same file, none a direct
  successor of the other). Ask the user, which version to use (the other one
  will be overwritten) - server or local. Asks for several files at once,
  but the user can make decisions for each file individually.

  @param download  boolean
  @param serverFiles  see result of readListingFile()  The versions of the
                             files as they are on the server.
  @param localFiles   see result of readListingFile()  The versions of the
                             files as they are in the local profile. Filenames
                             must match
  @result  Object with properties |server| and |local|. Each of them are of
                             the same format as the parameters. Each file
                             will be *either* in |local| or |server|.
                             If a fatal error occurred or the user pressed
                             Cancel, both arrays will be empty.
 */
function conflictAsk(download, serverFiles, localFiles)
{
  var result = new Object();
  result.server = new Array();
  result.local = new Array();
  try
  {
    // pass data to dialog
    // doc see mozSRoaming.cpp
    var ioParamBlock = Components
                   .classes["@mozilla.org/embedcomp/dialogparam;1"]
                   .createInstance(Components.interfaces.nsIDialogParamBlock);
    ioParamBlock.SetInt(0, download ? 1 : 2);
    /* how do I transfer the dates of the file versions to the dialog, without
       excluding C++ callers? XXX */
    ioParamBlock.SetInt(1, serverFiles.length);
    for (var i = 0, l = serverFiles.length; i < l; i++)
      ioParamBlock.SetString(i, serverFiles[i].filename);

    // invoke dialog, wait for return
    var windowWatcher =
               Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                         .getService(Components.interfaces.nsIWindowWatcher);
    windowWatcher.OpenWindow(window, kConflDlg, null,
                             "centerscreen,chrome,modal,titlebar",
                             ioParamBlock);
    // XXX what do we get in the case of Cancel by user?

    // Read result from dialog
    /* I am assuming that the sequence of iteration here is the same as in the
       last |for| statement. If that is not true, the indices gotten from
       param block will not match the array and we will interpret the result
       wrongly. */
    for (var i = 0, l = serverFiles.length; i < l; i++)
    {
      var value = ioParamBlock.GetInt(i);
      if (value == 1) // use server version
        result.server.push(serverFiles[i]);
      else if (value == 2) // use local version
        result.local.push(serverFiles[i]); // *cough*
    }
  }
  catch (e)
  {}
  return result;
}


/* Create nsIFile
   @param file  String  filename/path relative to the cache dir
   @result nsIFile
*/
function makeNSIFileFromRelFilename(filename)
{
  return GetIOService().newURI(gLocalDir + filename, null, null)
                       .QueryInterface(Components.interfaces.nsIFileURL)
                       .file.clone();
}


/* Moves a file in the local profile dir to another name there.
 */
function move(from, to)
{
  var lf = makeNSIFileFromRelFilename(from);
  if (lf && lf.exists())
    lf.moveTo(null, to);
}

















/*
  Takes 2 lists of files and compares them with each other and returns,
  which entries match and which don't.

  @param files1  See result of readListingFile()
  @param files2  See result of readListingFile()
  @result  Object with propreties |matches| and |mismatches|, both of which
                    have the same format as files1, i.e. an array of objects.
*/
function compareFilesOld(files1, files2)
{
  ddump("comparing file lists");

  var result = new Object();
  result.matches = new Array();
  result.mismatches = new Array();

  if (!files1 || !files2)
  {
    if (files1)
      result.mismatches = files1;
    else if (files2)
      result.mismatches = files2;
    return result;
  }

  /* go through files1 and compare each entry with the corresponding one from
     files2 */
  for (var i = 0, l = files1.length; i < l; i++)
  {
    var f1 = files1[i];
    var matches = false;

    // find corresponding entry in files2
    var f1fn = f1.filename; // faster?
    for (var i2 = 0, l2 = files2.length; i2 < l2; i2++)
    {
      if (files2[i2].filename == f1fn)
        matches = (f1.size == files2[i2].size
                   && f1.lastModified == files2[i2].lastModified
                   && f1.mimetype == files2[i2].mimetype);
    }

    ddump(f1fn + (matches ? " matches" : " doesn't match"));
    if (matches)
      result.matches.push(f1);
    else
      result.mismatches.push(f1);
  }

  // now go through files2 and find those not in files1
  // might not be the most efficient method
  for (i = 0, l = files2.length; i < l; i++)
  {
    // find corresponding entry in files1
    var found = false;
    var f2fn = files2[i].filename; // faster
    for (var i2 = 0, l2 = files1.length; i2 < l2; i2++)
    {
      if (files1[i2].filename == f2fn)
        found = true;
    }
    if (!found)
    {
      ddump(f2fn + " doesn't match (only in files2)");
      result.mismatches.push(f2);
    }
  }
  return result;
}


/* After the tranfer, checks, if the files are OK and performs appropriate
   actions, if necessary, like removing broken files
   @param transfer  Transfer object
*/
function checkFailedFiles(transfer)
{
  if (!transfer)
    // not fatal, will happen, if there are no files to be downloaded
    return;
  ddump("checking for failed files");

  for (var i = 0, l = transfer.files.length; i < l; i++)
  {
    var file = transfer.files[i];
    // XXX that won't work, the record is what we passed in, but hasn't been
    // updated after the transfer.
    if (file.status == "failed")
    {
      var lf = makeNSIFileFromRelFilename(file.filename);
      if (!lf)
      {
        dumpError("couldn't create localfile instance for " + file.filename);
      }
      else if (lf.exists())
      {
        ddump("removing failed file " + file.filename + " (" + lf.path + ")");
        lf.remove(false); // see above why false
        // XXX should we? or just record it in listing-local.xml?
      }
    }
    else if (file.status == "done")
    {
      if (!localFileOK(file))
      {
        ddumpCont("WARNING: File seems to be corrupted, but was reported as");
        ddump("succeeded.");
        ddump("Removing corrupted file " + file.filename + " ("+lf.path+")");
        lf.remove(false); // see above why false
        // XXX should we? or just record it in listing-local.xml?
      }
      
    }
  }
}


/*
  checks, if the local file matches the files list entry and is also otherwise
  sane. If it isn't, wipe it (XXX should we?).
  @param file  Object  See readListingFile (single entry from that array)
  @result bool  true, if the file is OK; false, if it has to be (re)transfered
 */
function localFileOK(file)
{
    var lf = makeNSIFileFromRelFilename(file.filename);
    if (!lf)
    {
      dumpError("couldn't create localfile instance for " + file.filename);
      return;
    }
    ddump(" checking " + file.filename + " (" + lf.path + ") for corruption");

    var updateFile = true;
    if (lf.exists())
    {
      if (lf.isFile() && lf.isReadable() && lf.isWritable())
      {
        ddumpCont("  lastmod: is " + lf.lastModifiedTime + ", should be ");
        ddump(file.lastModified);
        ddump("  size: is " + lf.fileSize + ", should be " + file.size);

        if (lf.lastModifiedTime >= file.lastModified &&
            lf.fileSize == file.size)
              /* Is the filesize garanteed to be exactly the same on all
                 platforms? Not that disk blocks or something that like
                 interferes. At least Mac's resource forks shouldn't.
                 XXX We have a problem, if the user's realtime clock is wrong.
                 We might force uncessesary updates or miss some.
                 Small variations are not that bad, esp. because I used >=,
                 not =. Still, we might need to use another approach, namely
                 to write a local listing.xml. */
        {
          ddump("  file doesn't need an update");
          updateFile = false;
        }
        else
        {
          ddump("  file needs update");
        }
      }
      else
      {
        ddump("  file has invalid charateristics - it's not a readable");
        ddump("  and writable file. I'm removing it. Removal will");
        ddump("  (intentionally) fail, if it's a directory with content.");
        lf.remove(false);
            /* Don't delete directories recursively.
               If there are bugs, we might wipe out the user's
               harddrive in the worst case, and this case here (is dir with
               content, should be file) is unlikely,
               so I don't think that's worth it. */
      }
    }
    else
    {
      ddump("  file doesn't exist yet");
    }

    return !updateFile;
}


/*
  Takes a list of files and compares them with the files in the
  local cache dir and figures out which files have to be transferred, i.e.
  checks, if their local counterparts have to be
  updated, because they are older or corrupted etc..

  @param filesList  See result of readListingFile()
  @result  See filesList
*/
function filesToBeUpdatedFilesystem(filesList)
{
  ddump("determining files to be updated");
  if (!filesList)
    return;

  var result = new Array();
  var i_result_array = 0;

  for (var i = 0, l = filesList.length; i < l; i++)
  {
    if (!localFileOK(filesList[i]))
    {
      ddump("  scheduling file for update");
      result[i_result_array++] = filesList[i];
    }
  }

  dumpObject(result, "to_be_updated");
  return result;
}
