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
 *  ntslapdregparms.h - NT Registry keys for Slapd.
 *
 ******************************************************/

#if defined( _WIN32 )

#if !defined( _NTSLAPDREGPARMS_H_ )
#define	_NTSLAPDREGPARMS_H_

#define COMPANY_KEY "SOFTWARE\\Netscape"
#define COMPANY_NAME		"Netscape"
#define PROGRAM_GROUP_NAME	"Netscape"
#define PRODUCT_NAME		"slapd"
#define PRODUCT_BIN			"ns-slapd"
#define SLAPD_EXE		    "slapd.exe"
#define SERVICE_EXE		    SLAPD_EXE
#define	SLAPD_CONF			"slapd.conf"
#define	MAGNUS_CONF			SLAPD_CONF
#define SLAPD_DONGLE_FILE	"password.dng"
#define DONGLE_FILE_NAME	SLAPD_DONGLE_FILE
#define PRODUCT_VERSION		"1.0"
#define EVENTLOG_APPNAME	"NetscapeSlapd"
#define DIRECTORY_SERVICE_PREFIX	"Netscape Directory Server "
#define SERVICE_PREFIX		DIRECTORY_SERVICE_PREFIX
#define CONFIG_PATH_KEY		"ConfigurationPath"
#define EVENTLOG_MESSAGES_KEY "EventMessageFile"
#define EVENT_LOG_KEY		"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"
#define ADMIN_REGISTRY_ROOT_KEY "Admin Server"
#define SLAPD_REGISTRY_ROOT_KEY	"Slapd Server"
#define PRODUCT_KEY			SLAPD_REGISTRY_ROOT_KEY
#endif /* _NTSLAPDREGPARMS_H_ */

#endif /* _WIN32 */
