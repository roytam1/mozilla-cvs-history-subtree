/******************************************************
 *
 *  Copyright (c) 1996 Netscape Communications Corp.
 *  This code is proprietary and is a trade secret of
 *  Netscape Communications Corp.
 *
 *  proto-ntutil.h - Prototypes for utility functions used
 *  throughout slapd on NT.
 *
 ******************************************************/
#if defined( _WINDOWS )

#ifndef _PROTO_NTUTIL
#define _PROTO_NTUTIL

/* 
 *
 * ntreg.c  
 *
 */
extern int SlapdGetRegSZ( LPTSTR lpszRegKey, LPSTR lpszValueName, LPTSTR lpszValue );

/* 
 *
 * getopt.c  
 *
 */
extern int getopt (int argc, char *const *argv, const char *optstring);

/* 
 *
 * ntevent.c  
 *
 */
extern BOOL MultipleInstances();
extern BOOL SlapdIsAService();
extern void InitializeSlapdLogging( LPTSTR lpszRegLocation, LPTSTR lpszEventLogName, LPTSTR lpszMessageFile );
extern void ReportSlapdEvent(WORD wEventType, DWORD dwIdEvent, WORD wNumInsertStrings, 
						char *pszStrings);
extern BOOL ReportSlapdStatusToSCMgr(
					SERVICE_STATUS *serviceStatus,
					SERVICE_STATUS_HANDLE serviceStatusHandle,
					HANDLE Event,
					DWORD dwCurrentState,
                    DWORD dwWin32ExitCode,
                    DWORD dwCheckPoint,
                    DWORD dwWaitHint);
extern void WINAPI SlapdServiceCtrlHandler(DWORD dwOpcode);
extern BOOL SlapdGetServerNameFromCmdline(char *szServerName, char *szCmdLine);

/* 
 *
 * ntgetpassword.c  
 *
 */
#ifdef NET_SSL
extern char *Slapd_GetPassword();
#ifdef FORTEZZA
extern char *Slapd_GetFortezzaPIN();
#endif
extern void CenterDialog(HWND hwndParent, HWND hwndDialog);
#endif /* NET_SSL */

#endif /* _PROTO_NTUTIL */

#endif /* _WINDOWS */
