#include <windows.h>
#include <aygshell.h>

void SentCommandRequest(const char* command, const char* value)
{
  // This will change in the future -- this window is for
  // the use of minimo_runner.exe only.

  
  char buffer[1024];
  COPYDATASTRUCT cds = { 0, 1024, buffer };
  
  if (strlen(command) != 3)
    return; // command string must be exactly 3 chars
  
  strcpy(buffer, command);
  strncat(buffer, value, 1000);
  
  HWND a = FindWindowW(L"MINIMO_LISTENER", NULL);

  int attempts = 20;
  while (!a && attempts)
  {
    attempts--;
    Sleep(500);
    a = FindWindowW(L"MINIMO_LISTENER", NULL);
  }

  SendMessage(a, WM_COPYDATA, NULL, (LPARAM)&cds); 
}


int main(int argc, char *argv[])
{
  SetCursor(LoadCursor(NULL,IDC_WAIT));

  HWND h = FindWindowW(NULL, L"Minimo");
  if (!h)
  {

#define MIN_MEMORY_TO_RUN 10*1024*1024
    MEMORYSTATUS mst;
    mst.dwLength  = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&mst);
    if (mst.dwAvailPhys < MIN_MEMORY_TO_RUN)
    {
      
      // Try to free memory by asking Shell to shutdown apps
      
      if (!SHCloseApps(MIN_MEMORY_TO_RUN))
        
      {
        
        // Handle the case where memory could not be freed
      }
    }

    char *cp;
    char exe[MAX_PATH];
    GetModuleFileName(GetModuleHandle(NULL), exe, sizeof(exe));
    cp = strrchr(exe,'\\');
    if (cp != NULL)
    {
      cp++; // pass the \ char.
      *cp = 0;
    }
    strcat(exe, "minimo.exe");
    
    PROCESS_INFORMATION pi;
    CreateProcess(exe, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, &pi);
  }
  else
  {
    SetForegroundWindow(h);
    ShowWindow(h, SW_SHOWNORMAL);
  }

  if (argc == 3)
  {
    if (!strcmp("-url", argv[1]))
      SentCommandRequest("URL", argv[2]);

    else if (!strcmp("-bm", argv[1]))
      SentCommandRequest("ABM", argv[2]);

  }

  SetCursor(NULL);
  return 0;
}

 
