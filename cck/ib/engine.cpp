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
  CString rootPath;
  CString templateDir;
  CString configname;
  CString configpath;
  CString che_path;
  CString che_file;
  
  if(!strcmp(argv[1], "-c"))
  {
    //The option "-c" means that ibengine.exe was called from wizardmachine.exe
    configPath = argv[2];
    FillGlobalWidgetArray(configPath);
  }
  else if(!strcmp(argv[1], "-u"))
  {
    //The option "-u" means that the user is running ibengine.exe via command line
    configPath = argv[2];
    FillGlobalWidgetArray(configPath);
    rootPath = GetModulePath();
    templateDir = rootPath + "WSTemplate";
    configname = GetGlobal("_NewConfigName");
    configpath = rootPath + "Configs\\" + configname;

    //Grab exact name of the .che file from configPath
    che_file = configPath;
    int extractposition = che_file.ReverseFind('\\');
    extractposition++;
    extractposition = (che_file.GetLength()) - extractposition;
    che_file = che_file.Right(extractposition);

    //These are some commands that we only want to run if ibengine.exe is run at command line

    //Create the config path
    _mkdir(configpath);

    //Copy files and directories from WSTemplate to new config directory
    CopyDir(templateDir, configpath, NULL, FALSE);

    //Copy the .che file given on command line to the appropriate directory
    che_path = configpath + "\\" + che_file;
    CopyFile(configPath, che_path, FALSE);
  }
  else
  {
    printf("\nYou have supplied incorrect command line options. \n"
           "Please, run either \"WizardMachine.exe\" or \"ibengine.exe -u <your_config_file_path>\"\n");
    return 1;
  } 
      

  StartIB();

  CString root = GetGlobal("Root");
  CString configName = GetGlobal("_NewConfigName");

  printf("\nInstaller creation is complete. The build is in %sConfigs\\%s\\Output\n", root, configName);
  return 0;
}
