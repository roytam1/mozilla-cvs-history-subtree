#include "stdafx.h"
#include <Winbase.h>
#include <direct.h>
#include <stdio.h>
#include "ib.h"
#include "globals.h"
#include "fstream.h"
#include <afxtempl.h>
#include <afxdisp.h>
#include "resource.h"
#include "NewDialog.h"

int main(int argc, char *argv[])
{
  CString configPath;
  for(int i=0; i < argc; i++)
  {
    if(!strcmp(argv[i], "-c"))
    {
      configPath = argv[i+1];
    }
  }
  BOOL result = FillGlobalWidgetArray(configPath); 
  int returnit = StartIB();
  printf("\nFinished StartIB!!\n");
  return 0;
}
