/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

/******************************************************
 *
 *  ntwatchdog.h - Defs for NT Watchdog Service.
 *
 ******************************************************/

#if defined( _WIN32 )

#if !defined( _NTWATCHDOG_H_ )
#define	_NTWATCHDOG_H_

#define FILE_PATHSEP '/'

#define SLAPD_ROOT             "SLAPD_ROOT"   // environment variable holding server root path
#define MORTALITY_KEY          "MortalityTimeSecs"
#define MINRAMFREE_KEY         "MinRamFree"
#define MINRAMTOTAL_KEY        "MinRamTotal"
#define MINRAMPERSERVER_KEY    "MinRamPerServer"
#define DEFAULT_MORTALITY_TIME  60              // seconds after startup up until server will NOT be restarted
#define DEFAULT_KILL_TIME       60              // seconds to wait for httpd.exe to shutdown
#define DEFAULT_CRON_TIME       60              // seconds to wait before rechecking cron.conf
#define DEFAULT_RESTART_TIME    10              // seconds to wait before restarting server
#define DEFAULT_MINRAMFREE      0               // KB free physical memory remaining
#define DEFAULT_MINRAMTOTAL     (30 * 1024)     // KB free physical memory installed
#define DEFAULT_MINRAMPERSERVER (15 * 1024)     // KB free physical memory per server

#define MSG_RESOURCES         "Not enough physical memory to start server."

// offsets for extra window bytes, used in Set/GetWindowLong()
#define GWL_PROCESS_HANDLE  (sizeof(LONG) * 0)
#define GWL_PASSWORD_ADDR   (sizeof(LONG) * 1)
#define GWL_PASSWORD_LENGTH (sizeof(LONG) * 2)

#define MAX_LINE      512
#define MAX_PASSWORD  256

#define CLOSEHANDLE(X) \
{ \
	if(X) \
	{ \
		CloseHandle(X); \
		X = 0; \
	} \
}

// in ntcron.c
LPTHREAD_START_ROUTINE CRON_ThreadProc(HANDLE hevWatchDogExit);

// in watchdog.c
BOOL WD_SysLog(WORD fwEventType, DWORD IDEvent, char *szData);

#endif /* _NTWATCHDOG_H_ */
#endif /* _WIN32 */
