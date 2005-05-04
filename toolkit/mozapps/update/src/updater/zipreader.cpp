#define STANDALONE
#include "zipstub.h"
#include "nsZipArchive.h"

#include "zipreader.h"
#include "errors.h"

int
ZipReader::Open(const char *path)
{
  if (mArchive)
    Close();

  mArchive = new nsZipArchive();
  if (!mArchive)
    return MEM_ERROR;

  if (mArchive->OpenArchive(path))
    return IO_ERROR;

  return OK;
}

void
ZipReader::Close()
{
  if (mArchive) {
    delete mArchive;
    mArchive = NULL;
  }
}

int
ZipReader::ExtractFile(const char *item, const char *dest)
{
  if (!mArchive)
    return USAGE_ERROR;

  if (mArchive->ExtractFile(item, dest, mArchive->GetFd()))
    return IO_ERROR;

  return OK;
}
