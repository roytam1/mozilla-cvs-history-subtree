This implements 4.x-like roaming.

To make the implementation vastly more simple, it has been decided that no syncing during the session happens. The design will not allow that either (at most sync in certain intervalls). A full-blown dynamic implementation that immediately update the server when a data change occured requires the cooperation of the data providers (bookmarks, prefs etc.) and is thus a huge change that I will leave to somebody else to implement independant of this roaming support here. alecf made such proposals a longer time ago on n.p.m.prefs, they sounded very interesting, but unfortunately, nobody implemented them so far.

When the users selected a profile, we will check, if it's a roaming profile and where the data lies. If necessary, we will contact the server and download the data as files. We will overwrite local profile files with the downloaded ones. Then, the profile works as if it were fully local. When the user then logs out (shuts down Mozilla or switches to another profile), we upload the local files, overwriting those on the server.

Following Conrad Carlen's advise, I do not hook up using nsIProfileChangeStatus, but in nsProfile directly. That just calls |mozISRoaming|. Its implementation uses various protocol handlers like |mozSRoamingCopy| to do the upload/download. These in turn may use generic protocol handlers like the netwerk HTTP protocol to do that.

Also following Conrad's advise, I do not store the roaming prefs in the prefs system (prefs.js etc.), because that it not yet initialized when I need the data (of course - prefs.js, user.js etc. might get changed by us), but in the Mozilla application registry. For the structure, see the comment at the top of prefs/top.js.


Overview of implementation:
- transfer.js (the Transfer class and support classes) contains the
  non-GUI logic to transfer files and track the progress and success.
- progressDialog.* shows the progress to the user (and also works as interface
  to the C++ code)
- conflictCheck.js is the "controller", controls the overall execution flow.
  It determines what has to be done (which files to transfer when etc.),
  including the conflict resolution logic, and kicks off the transfers.
- conflictResolution.* is a dialog to ask the user when we don't know which
  version of a file to use.



OUTDATED:
(I just scribbled this down, written thoughts, probably not very useful)

We compare the modification times of files and ask the user (-> GUI) before overwriting
newer files. To do that, we write the last modification time of the local files, according
to the OS, into a special file. That's called roaming-current.txt at first.
- If we upload
  1. First download the file roaming.txt from the server into the local file
      roaming-server.txt.
  2. Compare it with the pre-existing local file roaming-uploaded.txt.
    - If they match, the last upload worked OK and no other client uploaded in the
      meantime.
      1. Move roaming-current.txt to roaming-uploading.txt.
    - If they don't match, then another client uploaded files while the local client was
      running or the last uploaded failed in the middle.
      1. Ask user, which version of files to use.
      2. Create roaming-uploading.txt, merging roaming-current.txt and roaming-server.txt,
         matching user's selection.
      3. Delete roaming-current.txt
  3. Upload roaming files
  4. Upload local roaming-uploading.txt to roaming.txt on server.
  5. Delete 
  4. - If upload was successful, move roaming-uploading.txt to roaming-uploaded.txt
       and delete roaming-server.txt.
     - If upload failed, move roaming-uploading.txt to roaming-failed.txt
- If we download
  1. First download the file roaming.txt from the server into the local file
      roaming-server.txt.
  2. Compare it with the pre-existing local file roaming-uploaded.txt.
    - If they match, the last uploaded worked OK and no other client uploaded in the
      meantime.
    - If they don't match, then another client uploaded files while the local client was
      running or the last uploaded failed in the middle.
      Ask user, which version of files to use. XXX which roaming.txt do I upload?
    - If the files recorded in roaming-server.txt are older than those recorded in
      roaming-uploaded.txt, I don't know what happened.

We have the following cases:
- upload failed, one client
- download failed, one client
- upload of other client failed before we 
- download failed, several clients.
