#ifndef ZipReader_h__
#define ZipReader_h__

// This file provides a simplified API around nsZipArchive

class ZipReader
{
public:
  ZipReader() : mArchive(NULL) {}
  ~ZipReader() { Close(); }

  int Open(const char *path);
  void Close();

  int ExtractFile(const char *item, const char *destination);

private:
  class nsZipArchive *mArchive;
};

#endif  // ZipReader_h__
