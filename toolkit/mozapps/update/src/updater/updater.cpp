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
 * The Original Code is Mozilla Application Update.
 *
 * The Initial Developer of the Original Code is
 * Benjamin Smedberg <benjamin@smedbergs.us>
 *
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
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

/**
 *  update.manifest
 *  ---------------
 *
 *  contents = 1*( line )
 *  line     = method LWS *( param LWS ) CRLF
 *  method   = "add" | "remove" | "patch"
 *  CRLF     = "\r\n"
 *  LWS      = 1*( " " | "\t" )
 */

#include "zlib.h" // for crc32
#include "bspatch.h"
#include "progressui.h"
#include "zipreader.h"
#include "errors.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#if defined(XP_UNIX)
# include <sys/wait.h>
# include <unistd.h>
#elif defined(XP_WIN)
# include <windows.h>
# include <direct.h>
# include <io.h>
# define F_OK 00
# define W_OK 02
# define R_OK 04
# define access _access
# define snprintf _snprintf
# define fchmod(a,b)
#endif

#ifndef NULL
# define NULL (0)
#endif

#ifndef SSIZE_MAX
# define SSIZE_MAX LONG_MAX
#endif

#ifndef MAXPATHLEN
# ifdef MAX_PATH
#  define MAXPATHLEN MAX_PATH
# elif defined(_MAX_PATH)
#  define MAXPATHLEN _MAX_PATH
# elif defined(CCHMAXPATH)
#  define MAXPATHLEN CCHMAXPATH
# else
#  define MAXPATHLEN 1024
# endif
#endif

//-----------------------------------------------------------------------------

typedef void (* ThreadFunc)(void *param);

static int RunOnBackgroundThread(ThreadFunc func, void *param);

#ifdef XP_WIN
#include <process.h>

int
RunOnBackgroundThread(ThreadFunc func, void *param)
{
  if (_beginthread(func, 0, param) == -1)
    return MEM_ERROR;

  return 0;
}

#else

#include <pthread.h>

int
RunOnBackgroundThread(ThreadFunc func, void *param)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  pthread_t thr;
  int rv = pthread_create(&thr, &attr, (void* (*)(void *)) func, param);

  pthread_attr_destroy(&attr);
  return rv;
}

#endif

//-----------------------------------------------------------------------------

static char* gSourcePath;
static ZipReader gZipReader;

static const char kWhitespace[] = " \t";
static const char kNL[] = "\r\n";
static const char kQuote[] = "\"";
static const char kWSQuote[] = "\" \t";

static inline PRUint32
mmin(PRUint32 a, PRUint32 b)
{
  return (a > b) ? b : a;
}

static char*
mstrtok(const char *delims, char **str)
{
  if (!*str || !**str)
    return NULL;

  // skip leading "whitespace"
  char *ret = *str;
  const char *d;
  do {
    for (d = delims; *d != '\0'; ++d) {
      if (*ret == *d) {
        ++ret;
        break;
      }
    }
  } while (*d);

  if (!*ret) {
    *str = ret;
    return NULL;
  }

  char *i = ret;
  do {
    for (d = delims; *d != '\0'; ++d) {
      if (*i == *d) {
        *i = '\0';
        *str = ++i;
        return ret;
      }
    }
    ++i;
  } while (*i);

  *str = NULL;
  return ret;
}

// Ensure that the directory containing this file exists.
static int ensure_parent_dir(const char *path)
{
  char *slash = strrchr(path, '/');
  if (slash)
  {
    *slash = '\0';
    ensure_parent_dir(path);
#ifdef XP_WIN
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
    *slash = '/';
  }
  return OK;
}

static int copy_file(const char *spath, const char *dpath)
{
  int rv = ensure_parent_dir(dpath);
  if (rv)
    return rv;

  struct stat ss;

  int sfd = open(spath, O_RDONLY);
  if (sfd < 0 || fstat(sfd, &ss))
    return IO_ERROR;

  int dfd = open(dpath, O_WRONLY | O_TRUNC | O_CREAT, ss.st_mode);
  if (dfd < 0) {
    close(sfd);
    return IO_ERROR;
  }

  char buf[BUFSIZ];
  int sc;
  while ((sc = read(sfd, buf, sizeof(buf))) > 0) {
    int dc;
    char *bp = buf;
    while ((dc = write(dfd, bp, (unsigned int) sc)) > 0) {
      if ((sc -= dc) == 0)
        break;
      bp += dc;
    }
    if (dc < 0) {
      rv = IO_ERROR;
      goto end;
    }
  }
  if (sc < 0) {
    rv = IO_ERROR;
    goto end;
  }

end:
  close(sfd);
  close(dfd);

  return rv;
}

//-----------------------------------------------------------------------------
// LOGGING

static FILE *gLogFP = NULL;

static void LogInit()
{
  if (gLogFP)
    return;

  char logFile[MAXPATHLEN];
  snprintf(logFile, MAXPATHLEN, "%s/update.log", gSourcePath);

  gLogFP = fopen(logFile, "w");
}

static void LogFinish()
{
  if (!gLogFP)
    return;

  fclose(gLogFP);
  gLogFP = NULL;
}

static void LogPrintf(const char *fmt, ... )
{
  if (!gLogFP)
    return;

  va_list ap;
  va_start(ap, fmt);
  vfprintf(gLogFP, fmt, ap);
  va_end(ap);
}

#define LOG(args) LogPrintf args

//-----------------------------------------------------------------------------

#define BACKUP_EXT ".moz-backup"

// Create a backup copy of the specified file alongside it.
static int backup_create(const char *path)
{
  char backup[MAXPATHLEN];
  snprintf(backup, sizeof(backup), "%s" BACKUP_EXT, path);

  int rv = copy_file(path, backup);
  if (rv)
    return IO_ERROR;

  return OK;
}

// Copy the backup copy of the specified file back overtop
// the specified file.
static int backup_restore(const char *path)
{
  char backup[MAXPATHLEN];
  snprintf(backup, sizeof(backup), "%s" BACKUP_EXT, path);

  int rv = copy_file(backup, path);
  if (rv)
    return IO_ERROR;

  rv = remove(backup);
  if (rv)
    return IO_ERROR;

  return OK;
}

// Discard the backup copy of the specified file.
static int backup_discard(const char *path)
{
  char backup[MAXPATHLEN];
  snprintf(backup, sizeof(backup), "%s" BACKUP_EXT, path);

  int rv = remove(backup);
  if (rv)
    return IO_ERROR;

  return OK;
}

// Helper function for post-processing a temporary backup.
static void backup_finish(const char *path, int status)
{
  if (status == OK)
    backup_discard(path);
  else
    backup_restore(path);
}

//-----------------------------------------------------------------------------

// Because we're using a child thread to do all the real work
static int realmain();

static const int ACTION_DESCRIPTION_BUFSIZE = 256;

class Action
{
public:
  Action() : mNext(NULL) { }
  virtual ~Action() { }

  virtual int Parse(char *line) = 0;

  // Do any preprocessing to ensure that the action can be performed.  Execute
  // will be called if this Action and all others return OK from this method.
  virtual int Prepare() = 0;

  // Perform the operation.  Return OK to indicate success.  After all actions
  // have been executed, Finish will be called.  A requirement of Execute is
  // that it's operation be reversable from Finish.
  virtual int Execute() = 0;
  
  // Finish is called after execution of all actions.  If status is OK, then
  // all actions were successfully executed.  Otherwise, some action failed.
  virtual void Finish(int status) = 0;

private:
  Action* mNext;

  friend class ActionList;
};

class RemoveFile : public Action
{
public:
  RemoveFile() : mFile(NULL) { }

  int Parse(char *line);
  int Prepare();
  int Execute();
  void Finish(int status);

private:
  const char* mFile;
};

int
RemoveFile::Parse(char *line)
{
  mFile = mstrtok(kWhitespace, &line);
  if (!mFile)
    return PARSE_ERROR;

  return OK;
}

int
RemoveFile::Prepare()
{
  LOG(("PREPARE REMOVE %s\n", mFile));

  // We expect the file to exist if we are to remove it.
  int rv = access(mFile, F_OK);
  if (rv)
    return IO_ERROR;

  char *slash = strrchr(mFile, '/');
  if (slash) {
    *slash = '\0';
    rv = access(mFile, W_OK);
    *slash = '/';
  } else {
    rv = access(".", W_OK);
  }

  if (rv)
    return IO_ERROR;

  return OK;
}

int
RemoveFile::Execute()
{
  LOG(("EXECUTE REMOVE %s\n", mFile));

  // save a complete copy of the old file, and then remove the
  // old file.  we'll clean up the copy in Finish.

  int rv = backup_create(mFile);
  if (rv)
    return rv;

  rv = remove(mFile);
  if (rv)
    return IO_ERROR;

  return OK;
}

void
RemoveFile::Finish(int status)
{
  LOG(("FINISH REMOVE %s\n", mFile));

  backup_finish(mFile, status);
}

class AddFile : public Action
{
public:
  AddFile() : mFile(NULL) { }

  int Parse(char *line);
  int Prepare(); // check that the source file exists
  int Execute();
  void Finish(int status);

private:
  const char *mFile;
};

int
AddFile::Parse(char *line)
{
  mFile = mstrtok(kWhitespace, &line);
  if (!mFile)
    return PARSE_ERROR;

  return OK;
}

int
AddFile::Prepare()
{
  LOG(("PREPARE ADD %s\n", mFile));

/*
  int rv;

  char spath[MAXPATHLEN];
  snprintf(spath, MAXPATHLEN, "%s/%s", gSourcePath, mFile);

  rv = access(spath, R_OK);
  if (rv)
    return IO_ERROR;
*/

/* XXX maybe check to see if the directory is writable?
   XXX but we also have to worry about creating subdirectories if necessary
  rv = access(mFile, W_OK);
  if (rv)
    return IO_ERROR;
*/

  return OK;
}

int
AddFile::Execute()
{
  LOG(("EXECUTE ADD %s\n", mFile));

  //char spath[MAXPATHLEN];
  //snprintf(spath, MAXPATHLEN, "%s/%s", gSourcePath, mFile);

  int rv;

  // First make sure that we can actually get rid of any existing file.
  if (access(mFile, F_OK) == 0)
  {
    rv = backup_create(mFile);
    if (rv)
      return rv;

    rv = remove(mFile);
    if (rv)
      return IO_ERROR;
  }
  else
  {
    rv = ensure_parent_dir(mFile);
    if (rv)
      return rv;
  }
    
  //return copy_file(spath, mFile);
  return gZipReader.ExtractFile(mFile, mFile);
}

void
AddFile::Finish(int status)
{
  LOG(("FINISH ADD %s\n", mFile));

  backup_finish(mFile, status);
}

class PatchFile : public Action
{
public:
  PatchFile() : mPatchIndex(-1), pfd(-1), buf(NULL) { }
  ~PatchFile();

  int Parse(char *line);
  int Prepare(); // check for the patch file and for checksums
  int Execute();
  void Finish(int status);

private:
  int LoadSourceFile(int ofd);

  static int sPatchIndex;

  const char *mPatchFile;
  const char *mFile;
  int mPatchIndex;
  MBSPatchHeader header;
  int pfd;
  unsigned char *buf;
};

int PatchFile::sPatchIndex = 0;

PatchFile::~PatchFile()
{
  if (pfd >= 0)
    close(pfd);

  // delete the temporary patch file
  char spath[MAXPATHLEN];
  snprintf(spath, MAXPATHLEN, "%s/%d.bspatch", gSourcePath, mPatchIndex);
  remove(spath);

  free(buf);
}

int
PatchFile::LoadSourceFile(int ofd)
{
  struct stat os;
  int rv = fstat(ofd, &os);
  if (rv)
    return IO_ERROR;

  if (PRUint32(os.st_size) != header.slen)
    return BSP_ERROR_CORRUPT;

  buf = (unsigned char*) malloc(header.slen);
  if (!buf)
    return BSP_ERROR_NOMEM;

  int r = header.slen;
  unsigned char *rb = buf;
  while (r) {
    int c = read(ofd, rb, mmin(SSIZE_MAX,r));
    if (c < 0)
      return IO_ERROR;

    r -= c;

    if (c == 0 && r)
      return BSP_ERROR_CORRUPT;
  }

  // Verify that the contents of the source file correspond to what we expect.

  PRUint32 crc = crc32(0, NULL, 0);
  crc = crc32(crc, buf, header.slen);

  if (crc != header.scrc32)
    return BSP_ERROR_CORRUPT;
  
  return OK;
}

int
PatchFile::Parse(char *line)
{
  char *q;

  // format "<patchfile>" "<filetopatch>"
  mPatchFile = mstrtok(kWSQuote, &line);
  if (!mPatchFile)
    return PARSE_ERROR;

  q = mstrtok(kQuote, &line);
  if (!q)
    return PARSE_ERROR;

  mFile = mstrtok(kWSQuote, &line);
  if (!mFile)
    return PARSE_ERROR;

  return OK;
}

int
PatchFile::Prepare()
{
  // extract the patch to a temporary file
  mPatchIndex = sPatchIndex++;

  char spath[MAXPATHLEN];
  snprintf(spath, MAXPATHLEN, "%s/%d.bspatch", gSourcePath, mPatchIndex);

  remove(spath);

  int rv = gZipReader.ExtractFile(mPatchFile, spath);
  if (rv)
    return rv;

  pfd = open(spath, O_RDONLY);
  if (pfd < 0)
    return IO_ERROR;

  rv = MBS_ReadHeader(pfd, &header);
  if (rv)
    return rv;

  int ofd = open(mFile, O_RDONLY);
  if (ofd < 0)
    return IO_ERROR;

  rv = LoadSourceFile(ofd);
  close(ofd);

  return rv;
}

int
PatchFile::Execute()
{
  // Create backup copy of the destination file before proceeding.

  struct stat ss;
  if (stat(mFile, &ss))
    return IO_ERROR;

  int rv = backup_create(mFile);
  if (rv)
    return rv;

  rv = remove(mFile);
  if (rv)
    return IO_ERROR;

  int ofd = open(mFile, O_WRONLY | O_TRUNC | O_CREAT, ss.st_mode);
  if (ofd < 0)
    return IO_ERROR;

  return MBS_ApplyPatch(&header, pfd, buf, ofd);
}

void
PatchFile::Finish(int status)
{
  backup_finish(mFile, status);
}

#if 0
// Neat idea, but windows doesn't support fork().  Let's implement this
// feature when we actually need it.

class SubManifest : public Action
{
public:
  SubManifest() : mManifest(NULL) { }

  int Parse(char *line);
  int Prepare();
  int Execute();

private:
  char *mManifest;
};

int
SubManifest::Parse(char *line)
{
  mManifest = mstrtok(kWhitespace, &line);
  if (!mManifest)
    return PARSE_ERROR;

  return OK;
}

int
SubManifest::Prepare()
{
  char spath[MAXPATHLEN];
  snprintf(spath, MAXPATHLEN, "%s/%s/update.manifest", gSourcePath, mManifest);

  struct stat s;

  // All we do here is make sure that the manifest exists. We're under the
  // assumption that each manifest lists a relatively self-contained update
  // operation, so we can safely apply it even if the following operations fail.
  if (stat(spath, &s))
    return SOURCE_ERROR;

  return OK;
}

int SubManifest::Execute()
{
  pid_t pid = fork();
  switch (pid) {
  case -1:
    return IO_ERROR;

  case 0:
    char spath[MAXPATHLEN];
    snprintf(spath, MAXPATHLEN, "%s/%s", gSourcePath, mManifest);
    gSourcePath = spath;
    exit(realmain());

  default:
    int status;
    waitpid(pid, &status, 0);
    return status;
  }
}
#endif

#if 0
// XXXbsmedberg implement me!
class ExecuteBinary : public Action
{
public:
  ExecuteBinary(char *line);

  int Prepare(); // check that the binary exists
  int Execute();

private:
  char *mExecutable;
  char *mArg;
};
#endif

static int
LaunchCallbackApp(const char *cmdLine)
{
  // Someone will probably tell me that this is a bad idea, but for now it
  // seems like it fits the bill.  It saves us from having to parse the command
  // line in order to call execv, and it nicely blocks this process until the
  // command line is finished executing.

  return system(cmdLine);
}

static void
WriteStatusFile(int status)
{
  // This is how we communicate our completion status to the main application.

  char filename[MAXPATHLEN];
  snprintf(filename, MAXPATHLEN, "%s/update.status", gSourcePath);

  int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
  if (fd < 0)
    return;

  const char *text = (status == OK) ? "succeeded" : "failed";
  write(fd, text, strlen(text));
  write(fd, kNL, sizeof(kNL)-1);
  close(fd);
}

static void
threadfunc(void *param)
{
  // open ZIP archive and process...

  char dataFile[MAXPATHLEN];
  snprintf(dataFile, MAXPATHLEN, "%s/update.jar", gSourcePath);

  int rv = gZipReader.Open(dataFile);
  if (rv == OK) {
    rv = realmain();
    gZipReader.Close();
  }

  if (rv)
    LOG(("failed: %d\n", rv));
  else
    LOG(("succeeded\n"));
  WriteStatusFile(rv);

  QuitProgressUI();
}

int main(int argc, char **argv)
{
#if defined(XP_WIN)
  // XXX This is an ugly hack to "ensure" that the parent process has exited
  //     before we begin patching.  We need to do something better for sure.
  Sleep(100);
#endif

  int rv = InitProgressUI(&argc, &argv);
  if (rv)
    return rv;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <path> [callback]\n", argv[0]);
    return 1;
  }

  gSourcePath = argv[1];

  LogInit();

  rv = RunOnBackgroundThread(threadfunc, NULL);
  if (rv == 0)
    rv = ShowProgressUI();

  LogFinish();

  if (argc >= 3)
    return LaunchCallbackApp(argv[2]);

  return rv;
}

class ActionList
{
public:
  ActionList() : mFirst(NULL), mLast(NULL), mCount(0) { }
  ~ActionList();

  void Append(Action* action);
  int Prepare();
  int Execute();
  void Finish(int status);

private:
  Action *mFirst;
  Action *mLast;
  int     mCount;
};

ActionList::~ActionList()
{
  Action* a = mFirst;
  while (a) {
    Action *b = a;
    a = a->mNext;
    delete b;
  }
}

void
ActionList::Append(Action *action)
{
  if (mLast)
    mLast->mNext = action;
  else
    mFirst = action;

  mLast = action;
  mCount++;
}

int
ActionList::Prepare()
{
  Action *a = mFirst;
  while (a) {
    int rv = a->Prepare();
    if (rv)
      return rv;

    a = a->mNext;
  }

  UpdateProgressUI(1.0f);

  return OK;
}

int
ActionList::Execute()
{
  int i = 0;
  float divisor = mCount / 98.0f;

  Action *a = mFirst;
  while (a) {
    UpdateProgressUI(1.0f + float(i++) / divisor);

    int rv = a->Execute();
    if (rv)
    {
      LOG(("### execution failed\n"));
      return rv;
    }

    a = a->mNext;
  }

  return OK;
}

void
ActionList::Finish(int status)
{
  Action *a = mFirst;
  while (a) {
    a->Finish(status);
    a = a->mNext;
  }

  UpdateProgressUI(100.0f);
}

int realmain()
{
  char manifest[MAXPATHLEN];
  snprintf(manifest, MAXPATHLEN, "%s/update.manifest", gSourcePath);

  // extract the manifest
  int rv = gZipReader.ExtractFile("update.manifest", manifest);
  if (rv)
    return rv;

  int mfd = open(manifest, O_RDONLY);
  if (mfd < 0)
    return -1;

  struct stat ms;
  rv = fstat(mfd, &ms);
  if (rv)
    return IO_ERROR;

  char *mbuf = (char*) malloc(ms.st_size + 1);
  if (!mbuf)
    return BSP_ERROR_NOMEM;

  int r = ms.st_size;
  char *rb = mbuf;
  while (r) {
    int c = read(mfd, rb, mmin(SSIZE_MAX,r));
    if (c < 0)
      return IO_ERROR;

    r -= c;
    rb += c;

    if (c == 0 && r)
      return BSP_ERROR_CORRUPT;
  }
  mbuf[ms.st_size] = '\0';

  ActionList list;

  rb = mbuf;
  char *line;
  while((line = mstrtok(kNL, &rb)) != 0) {
    // skip comments
    if (*line == '#')
      continue;

    char *token = mstrtok(kWhitespace, &line);
    if (!token)
      return PARSE_ERROR;

    Action *action = NULL;
    if (strcmp(token, "remove") == 0) {
      action = new RemoveFile();
    }
    else if (strcmp(token, "add") == 0) {
      action = new AddFile();
    }
    /*
    else if (strcmp(token, "submanifest") == 0) {
      action = new SubManifest();
    }
    else if (strcmp(token, "replace") == 0) {
      action = new ReplaceFile();
    }
    */
    else if (strcmp(token, "patch") == 0) {
      action = new PatchFile();
    }
    else {
      return PARSE_ERROR;
    }

    if (!action)
      return BSP_ERROR_NOMEM;

    rv = action->Parse(line);
    if (rv)
      return rv;

    list.Append(action);
  }

  rv = list.Prepare();
  if (rv)
    return rv;

  rv = list.Execute();

  list.Finish(rv);
  return rv;
}

#if defined(XP_WIN) && !defined(DEBUG)
// We need WinMain in order to not be a console app.  This function is unused
// if we are a console application.
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR args, int )
{
  // Do the real work.
  return main(__argc, __argv);
}
#endif
