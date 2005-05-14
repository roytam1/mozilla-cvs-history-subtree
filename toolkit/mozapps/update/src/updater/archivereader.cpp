#include <string.h>
#include "bzlib.h"
#include "archivereader.h"
#include "errors.h"

int
ArchiveReader::Open(const char *path)
{
  if (mArchive)
    Close();

  mArchive = mar_open(path);
  if (!mArchive)
    return IO_ERROR;

  return OK;
}

void
ArchiveReader::Close()
{
  if (mArchive) {
    mar_close(mArchive);
    mArchive = NULL;
  }
}

int
ArchiveReader::ExtractFile(const char *name, const char *dest)
{
  const MarItem *item = mar_find_item(mArchive, name);
  if (!item)
    return IO_ERROR;

  FILE *fp = fopen(dest, "wb");
  if (!fp)
    return IO_ERROR;

  int rv = ExtractItemToStream(item, fp);

  fclose(fp);
  return rv;
}

int
ArchiveReader::ExtractItemToStream(const MarItem *item, FILE *fp)
{
  /* decompress the data chunk by chunk */

  char inbuf[BUFSIZ], outbuf[BUFSIZ];
  bz_stream strm;
  int offset, inlen, ret = OK;

  memset(&strm, 0, sizeof(strm));
  if (BZ2_bzDecompressInit(&strm, 0, 0) != BZ_OK)
    return UNEXPECTED_ERROR;

  offset = 0;
  for (;;) {
    if (offset < item->length && strm.avail_in == 0) {
      inlen = mar_read(mArchive, item, offset, inbuf, BUFSIZ);
      if (inlen <= 0)
        return -1;
      offset += inlen;
      strm.next_in = inbuf;
      strm.avail_in = inlen;
    }

    strm.next_out = outbuf;
    strm.avail_out = BUFSIZ;

    ret = BZ2_bzDecompress(&strm);
    if (ret != BZ_OK && ret != BZ_STREAM_END) {
      ret = IO_ERROR;
      break;
    }

    if (strm.avail_out < BUFSIZ) {
      if (fwrite(outbuf, BUFSIZ - strm.avail_out, 1, fp) != 1) {
        ret = IO_ERROR;
        break;
      }
    }

    if (ret == BZ_STREAM_END) {
      ret = OK;
      break;
    }
  }

  BZ2_bzDecompressEnd(&strm);
  return ret;
}
