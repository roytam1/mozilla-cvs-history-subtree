/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Bill Law    <law@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include <windows.h>
#include <string.h>

#include "nsString.h"
#include "nsINativeAppSupportWin.h"
#include "nsIStringBundle.h"

// Where Mozilla stores its own registry values.
LPCTSTR const mozillaKeyName = _T("Software\\Mozilla\\Desktop");

static const TCHAR shortcutSuffix[] = _T(" -url \"%1\"");
static const TCHAR chromeSuffix[] = _T(" -chrome \"%1\"");

//
//  Control the string type we use based on wether or not the win32 API
//      defaults to UNICODE or not.
//
#if !defined(UNICODE)
typedef nsCString reString;
typedef nsCAutoString reAutoString;
#define RE_LITERAL_STRING NS_LITERAL_CSTRING
#define RE_NAMED_LITERAL_STRING NS_NAMED_LITERAL_CSTRING
#else /* UNICODE */
typedef nsString reString;
typedef nsAutoString reAutoString;
#define RE_LITERAL_STRING NS_LITERAL_STRING
#define RE_NAMED_LITERAL_STRING NS_NAMED_LITERAL_STRING
#endif /* UNICODE */

// Returns the (fully-qualified) name of this executable.
static reString thisApplication() {
    static reAutoString result;

    if ( result.IsEmpty() ) {
        TCHAR buffer[MAX_PATH] = { 0 };
    	DWORD len = ::GetModuleFileName( NULL, buffer, sizeof buffer );
        len = ::GetShortPathName( buffer, buffer, sizeof buffer );
    
#if !defined(UNICODE)
        result.AssignWithConversion(buffer);
#else /* UNICODE */
        result.Assign(buffer);
#endif /* UNICODE */

        ToUpperCase(result);
    }

    return result;
}

// Returns the "short" name of this application (in upper case).  This is for
// use as a StartMenuInternet value.
static reString shortAppName() {
    static reAutoString result;
    
    if ( result.IsEmpty() ) { 
        // Find last backslash in thisApplication().
        reAutoString thisApp( thisApplication() );
        PRInt32 n = thisApp.RFind( "\\" );
        if ( n != kNotFound ) {
            // Use what comes after the last backslash.
            result = (LPCTSTR)thisApp.get() + n + 1;
        } else {
            // Use entire string.
            result = thisApp;
        }
    }

    return result;
}

// RegistryEntry
//
// Generic registry entry (no saving of previous values).  Each is comprised of:
//      o A base HKEY
//      o A subkey name.
//      o An optional value name (empty for the "default" value).
//      o The registry setting we'd like this entry to have when set.
struct RegistryEntry {
    HKEY        baseKey;   // e.g., HKEY_CURRENT_USER
    PRBool      isNull;    // i.e., should use ::RegDeleteValue
    reString    keyName;   // Key name.
    reString    valueName; // Value name (can be empty, which implies NULL).
    reString    setting;   // What we set it to.

    RegistryEntry( HKEY baseKey, LPCTSTR keyName, LPCTSTR valueName, LPCTSTR setting )
        : baseKey( baseKey ), isNull( setting == 0 ), keyName( keyName ), valueName( valueName ), setting( setting ? setting : _T("") ) {
    }

    PRBool     isAlreadySet() const;
    nsresult   set();
    nsresult   reset();
    reString  currentSetting( PRBool *currentUndefined = 0 ) const;

    // Return value name in proper form for passing to ::Reg functions
    // (i.e., emptry string is converted to a NULL pointer).
    LPCTSTR valueNameArg() const {
        return valueName.IsEmpty() ? NULL : valueName.get();
    }

    reString  fullName() const;
};

// BoolRegistryEntry
// 
// These are used to store the "windows integration" preferences.
// You can query the value via operator void* (i.e., if ( boolPref )... ).
// These are stored under HKEY_LOCAL_MACHINE\Software\Mozilla\Desktop.
// Set sets the stored value to "1".  Reset deletes it (which implies 0).
struct BoolRegistryEntry : public RegistryEntry {
    BoolRegistryEntry( LPCTSTR name )
        : RegistryEntry( HKEY_LOCAL_MACHINE, mozillaKeyName, name, _T("1") ) {
    }
    operator PRBool();
};

// SavedRegistryEntry
//
// Like a plain RegistryEntry, but set/reset save/restore the
// it had before we set it.
struct SavedRegistryEntry : public RegistryEntry {
    SavedRegistryEntry( HKEY baseKey, LPCTSTR keyName, LPCTSTR valueName, LPCTSTR setting )
        : RegistryEntry( baseKey, keyName, valueName, setting ) {
    }
    nsresult set();
    nsresult reset();
};

// ProtocolRegistryEntry
//
// For setting entries for a given Internet Shortcut protocol.
// The key name is calculated as
// HKEY_LOCAL_MACHINE\Software\Classes\protocol\shell\open\command.
// The setting is this executable (with appropriate suffix).
// Set/reset are trickier in this case.
struct ProtocolRegistryEntry : public SavedRegistryEntry {
    reString protocol;
    ProtocolRegistryEntry( LPCTSTR protocol )
        : SavedRegistryEntry( HKEY_LOCAL_MACHINE, _T(""), _T(""), thisApplication().get() ),
          protocol( protocol ) {
        keyName = _T("Software\\Classes\\");
        keyName += protocol;
        keyName += _T("\\shell\\open\\command");

        // Append appropriate suffix to setting.
        if ( this->protocol.Equals( _T("chrome") ) || this->protocol.Equals( _T("MozillaXUL") ) ) {
            // Use "-chrome" command line flag.
            setting += chromeSuffix;
        } else {
            // Use standard "-url" command line flag.
            setting += shortcutSuffix;
        }
    }
    nsresult set();
    nsresult reset();
};

// DDERegistryEntry
//
// Like a protocol registry entry, but for the shell\open\ddeexec subkey.
//
// We need to remove this subkey entirely to ensure we work properly with
// various programs on various platforms (see Bugzilla bugs 59078, 58770, etc.).
//
// We don't try to save everything, though.  We do save the known useful info
// under the ddeexec subkey:
//     ddexec\@
//     ddeexec\NoActivateHandler
//     ddeexec\Application\@
//     ddeexec\Topic\@
//
// set/reset save/restore these values and remove/restore the ddeexec subkey
struct DDERegistryEntry : public SavedRegistryEntry {
    DDERegistryEntry( LPCTSTR protocol )
        : SavedRegistryEntry( HKEY_LOCAL_MACHINE, _T(""), _T(""), 0 ),
          activate( HKEY_LOCAL_MACHINE, _T(""), _T("NoActivateHandler"), 0 ),
          app( HKEY_LOCAL_MACHINE, _T(""), _T(""), 0 ),
          topic( HKEY_LOCAL_MACHINE, _T(""), _T(""), 0 ) {
        // Derive keyName from protocol.
        keyName = _T("Software\\Classes\\");
        keyName += protocol;
        keyName += _T("\\shell\\open\\ddeexec");
        // Set subkey names.
        activate.keyName = keyName;
        app.keyName = keyName;
        app.keyName += _T("\\Application");
        topic.keyName = keyName;
        topic.keyName += _T("\\Topic");
    }
    nsresult set();
    nsresult reset();
    SavedRegistryEntry activate, app, topic;
};

// FileTypeRegistryEntry
//
// For setting entries relating to a file extension (or extensions).
// This object itself is for the "file type" associated with the extension.
// Set/reset manage the mapping from extension to the file type, as well.
// The description is taken from defDescKey if available. Otherwise desc 
// is used.
struct FileTypeRegistryEntry : public ProtocolRegistryEntry {
    reString fileType;
    const TCHAR **ext;
    reString desc;
    reString defDescKey;
    FileTypeRegistryEntry ( const TCHAR **ext, LPCTSTR fileType, 
        LPCTSTR desc, LPCTSTR defDescKey )
        : ProtocolRegistryEntry( fileType ),
          fileType( fileType ),
          ext( ext ),
          desc( desc ),
          defDescKey(defDescKey) {
    }
    nsresult set();
    nsresult reset();
};

// EditableFileTypeRegistryEntry
//
// Extends FileTypeRegistryEntry by setting an additional handler for an "edit" command.
struct EditableFileTypeRegistryEntry : public FileTypeRegistryEntry {
    EditableFileTypeRegistryEntry( const TCHAR **ext, LPCTSTR fileType, 
        LPCTSTR desc, LPCTSTR defDescKey )
        : FileTypeRegistryEntry( ext, fileType, desc, defDescKey ) {
    }
    nsresult set();
};

// Generate the "full" name of this registry entry.
reString RegistryEntry::fullName() const {
    reString result;
    if ( baseKey == HKEY_CURRENT_USER ) {
        result = _T("HKEY_CURRENT_USER\\");
    } else if ( baseKey == HKEY_LOCAL_MACHINE ) {
        result = _T("HKEY_LOCAL_MACHINE\\");
    } else {
        result = _T("\\");
    }
    result += keyName;
    if ( !valueName.IsEmpty() ) {
        result += _T("[");
        result += valueName;
        result += _T("]");
    }
    return result;
}

// Tests whether registry entry already has desired setting.
PRBool RegistryEntry::isAlreadySet() const {
    PRBool result = FALSE;

    reAutoString current( currentSetting() );

    result = ( current == setting );

    return result;
}

// Gives registry entry the desired setting.
nsresult RegistryEntry::set() {
#ifdef DEBUG_law
if ( isNull && setting.IsEmpty() ) printf( "Deleting %s\n", fullName().get() );
else printf( "Setting %s=%s\n", fullName().get(), setting.get() );
#endif
    nsresult result = NS_ERROR_FAILURE;

    HKEY   key;
    LONG   rc = ::RegOpenKey( baseKey, keyName.get(), &key );

    // See if key doesn't exist yet...
    if ( rc == ERROR_FILE_NOT_FOUND ) {
        rc = ::RegCreateKey( baseKey, keyName.get(), &key );
    }
    if ( rc == ERROR_SUCCESS ) {
        if ( isNull && setting.IsEmpty() ) {
            // This means we need to actually remove the value, not merely set it to an
            // empty string.
            rc = ::RegDeleteValue( key, valueNameArg() );
            if ( rc == ERROR_SUCCESS ) {
                result = NS_OK;
            }
        } else {
            // Get current value to see if it is set properly already.
            TCHAR buffer[4096] = { 0 };
            DWORD len = sizeof buffer;
            rc = ::RegQueryValueEx( key, valueNameArg(), NULL, NULL, (LPBYTE)buffer, &len );
            if ( rc != ERROR_SUCCESS || _tcscmp( setting.get(), buffer ) != 0 ) {
                rc = ::RegSetValueEx( key, valueNameArg(), NULL, REG_SZ, (LPBYTE)setting.get(), setting.Length() );
#ifdef DEBUG_law
NS_WARN_IF_FALSE( rc == ERROR_SUCCESS, fullName().get() );
#endif
                if ( rc == ERROR_SUCCESS ) {
                    result = NS_OK;
                }
            } else {
                // Already has desired setting.
                result = NS_OK;
            }
        }
        ::RegCloseKey( key );
    } else {
#ifdef DEBUG_law
NS_WARN_IF_FALSE( rc == ERROR_SUCCESS, fullName().get() );
#endif
    }
    return result;
}

// Get current setting, set new one, then save the previous.
nsresult SavedRegistryEntry::set() {
    nsresult rv = NS_OK;
    PRBool   currentlyUndefined = PR_TRUE;
    reAutoString prev( currentSetting( &currentlyUndefined ) );
    // See if value is changing.
    // We need an extra check for the case where we have an empty entry
    // and we need to remove it entirely.
    if ( setting != prev || ( !currentlyUndefined && isNull ) ) {
        // Set new.
        rv = RegistryEntry::set();
        if ( NS_SUCCEEDED( rv ) ) {
            // Save old.
            RegistryEntry( HKEY_LOCAL_MACHINE, _T("Software\\Mozilla\\Desktop"), fullName().get(), prev.get() ).set();
        }
    }
    return rv;
}

// setWindowsXP
//
//  We need to:
//    a. Make sure this application is registered as a "Start Menu
//       internet app" under HKLM\Software\Clients\StartMenuInternet.
//    b. Make this app the default "Start Menu internet app" for this
//       user.
static void setWindowsXP() {
    // We test for the presence of this subkey as a WindowsXP test.
    // We do it this way so that vagueries of future Windows versions
    // are handled as best we can.
    HKEY key;
    reAutoString baseKey( RE_LITERAL_STRING( "Software\\Clients\\StartMenuInternet" ) );
    LONG rc = ::RegOpenKey( HKEY_LOCAL_MACHINE, baseKey.get(), &key );
    if ( rc == ERROR_SUCCESS ) {
        // OK, this is WindowsXP (or equivalent).  Add this application to the
        // set registered as "Start Menu internet apps."  These entries go
        // under the subkey MOZILLA.EXE (or whatever the name of this executable is),
        // that subkey name is generated by the utility function shortAppName.
        if ( rc == ERROR_SUCCESS ) {
            // The next 3 go under this subkey.
            reAutoString subkey( baseKey + RE_LITERAL_STRING( "\\" ) + shortAppName() );
            // Pretty name.  This goes into the LocalizedString value.  It is the
            // name of the executable (preceded by '@'), followed by ",-nnn" where
            // nnn is the resource identifier of the string in the .exe.  That value
            // comes from nsINativeAppSupportWin.h.
            TCHAR buffer[ _MAX_PATH + 8 ]; // Path, plus '@', comma, minus, and digits (5)
            _sntprintf( buffer, sizeof buffer / sizeof(TCHAR), _T("@%s,-%d"), thisApplication().get(), IDS_STARTMENU_APPNAME );
            RegistryEntry( HKEY_LOCAL_MACHINE, 
                           subkey.get(),
                           _T("LocalizedString"), 
                           buffer ).set();
            // Default icon (from .exe resource).
            RegistryEntry( HKEY_LOCAL_MACHINE, 
                           reAutoString( subkey + RE_LITERAL_STRING( "\\DefaultIcon" ) ).get(),
                           _T(""), 
                           reAutoString( thisApplication() + RE_LITERAL_STRING( ",0" ) ).get() ).set();
            // Command to open.
            RegistryEntry( HKEY_LOCAL_MACHINE,
                           reAutoString( subkey + RE_LITERAL_STRING( "\\shell\\open\\command" ) ).get(),
                           _T(""), 
                           thisApplication().get() ).set();
            // "Properties" verb.  The default value is the text that will appear in the menu.
            // The default value under the command subkey is the name of this application, with
            // arguments to cause the Preferences window to appear.
            nsCOMPtr<nsIStringBundleService> bundleService( do_GetService( "@mozilla.org/intl/stringbundle;1" ) );
            nsCOMPtr<nsIStringBundle> bundle;
            nsXPIDLString label;
            if ( bundleService &&
                 NS_SUCCEEDED( bundleService->CreateBundle( "chrome://global-platform/locale/nsWindowsHooks.properties",
                                                       getter_AddRefs( bundle ) ) ) &&
                 NS_SUCCEEDED( bundle->GetStringFromName( NS_LITERAL_STRING( "prefsLabel" ).get(), getter_Copies( label ) ) ) ) {
                // Set the label that will appear in the start menu context menu.
                RegistryEntry( HKEY_LOCAL_MACHINE,
                               reAutoString( subkey + RE_LITERAL_STRING( "\\shell\\properties" ) ).get(),
                               _T(""), 
#if !defined(UNICODE)
                               NS_ConvertUCS2toUTF8( label ).get()
#else /* UNICODE */
                               label.get()
#endif /* UNICODE */
                               ).set();
            }
            RegistryEntry( HKEY_LOCAL_MACHINE,
                           reAutoString( subkey + RE_LITERAL_STRING( "\\shell\\properties\\command" ) ).get(),
                           _T(""), 
                           reAutoString( thisApplication() + RE_LITERAL_STRING( "-chrome \"chrome://communicator/content/pref/pref.xul\"" ) ).get() ).set();

            // Now we need to select our application as the default start menu internet application.
            // This is accomplished by first trying to store our subkey name in 
            // HKLM\Software\Clients\StartMenuInternet's default value.  See
            // http://support.microsoft.com/directory/article.asp?ID=KB;EN-US;Q297878 for detail.
            SavedRegistryEntry hklmAppEntry( HKEY_LOCAL_MACHINE, baseKey.get(), _T(""), shortAppName().get() );
            hklmAppEntry.set();
            // That may or may not have worked (depending on whether we have sufficient access).
            if ( hklmAppEntry.currentSetting() == hklmAppEntry.setting ) {
                // We've set the hklm entry, so we can delete the one under hkcu.
                SavedRegistryEntry( HKEY_CURRENT_USER, baseKey.get(), _T(""), 0 ).set();
            } else {
                // All we can do is set the default start menu internet app for this user.
                SavedRegistryEntry( HKEY_CURRENT_USER, baseKey.get(), _T(""), shortAppName().get() );
            }
            // Notify the system of the changes.
            ::SendMessage( HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Software\\Clients\\StartMenuInternet" );
        }
    }
}

// Set this entry and its corresponding DDE entry.  The DDE entry
// must be turned off to stop Windows from trying to use DDE.
nsresult ProtocolRegistryEntry::set() {
    // If the protocol is http, then we have to do special stuff.
    // We must take care of this first because setting the "protocol entry"
    // for http will cause WindowsXP to do stuff automatically for us,
    // thereby making it impossible for us to propertly reset.
    if ( protocol == RE_LITERAL_STRING( "http" ) ) {
        setWindowsXP();
    }

    // Set this entry.
    nsresult rv = SavedRegistryEntry::set();

    // Save and set corresponding DDE entry(ies).
    DDERegistryEntry( protocol.get() ).set();

    return rv;
}

// Not being a "saved" entry, we can't restore, so just delete it.
nsresult RegistryEntry::reset() {
    HKEY key;
    LONG rc = ::RegOpenKey( baseKey, keyName.get(), &key );
    if ( rc == ERROR_SUCCESS ) {
        rc = ::RegDeleteValue( key, valueNameArg() );
#ifdef DEBUG_law
if ( rc == ERROR_SUCCESS ) _tprintf( "Deleting key=%s\n", (LPCTSTR)fullName().get() );
#endif
    }
    return NS_OK;
}

// Resets registry entry to the saved value (if there is one).  We first
// ensure that we still "own" that entry (by comparing its value to what
// we would set it to).
nsresult SavedRegistryEntry::reset() {
    nsresult result = NS_OK;

    // Get current setting for this key/value.
    reAutoString current( currentSetting() );

    // Test if we "own" it.
    if ( current == setting ) {
        // Unset it, then.  First get saved value it had previously.
        PRBool noSavedValue = PR_TRUE;
        RegistryEntry saved = RegistryEntry( HKEY_LOCAL_MACHINE, mozillaKeyName, fullName().get(), _T("") );
        // There are 3 cases:
        //    - no saved entry
        //    - empty saved entry
        //    - a non-empty saved entry
        // We delete the current entry in the first case, and restore
        // the saved entry (empty or otherwise) in the other two.
        setting = saved.currentSetting( &noSavedValue );
        if ( !setting.IsEmpty() || !noSavedValue ) {
            // Set to previous value.
            isNull = PR_FALSE; // Since we're resetting and the saved value may be empty, we
                               // need to make sure set() doesn't mistakenly delete this entry.
            result = RegistryEntry::set();
            // Remove saved entry.
            saved.reset();
        } else {
            // No saved value, just delete this entry.
            result = RegistryEntry::reset();
        }
    }

    return result;
}

// resetWindowsXP
//
// This function undoes "setWindowsXP," more or less.  It only needs to restore the selected
// default Start Menu internet application.  The registration of this application as one of
// the start menu internet apps can remain.  There is no check for the presence of anything
// because the SaveRegistryEntry::reset calls will have no effect if there is no value at that
// location (or, if that value has been changed by another application).
static void resetWindowsXP() {
    RE_NAMED_LITERAL_STRING( baseKey, "Software\\Clients\\StartMenuInternet" );
    // First, try to restore the HKLM setting.  This will fail if either we didn't
    // set that, or, if we don't have access).
    SavedRegistryEntry( HKEY_LOCAL_MACHINE, baseKey.get(), _T(""), shortAppName().get() ).reset();

    // The HKCU setting is trickier.  We may have set it, but we may also have
    // removed it (see setWindowsXP(), above).  We first try to reverse the
    // setting.  If we had removed it, then this will fail.
    SavedRegistryEntry( HKEY_CURRENT_USER, baseKey.get(), _T(""), shortAppName().get() ).reset();
    // Now, try to reverse the removal of this key.  This will fail if there is a  current
    // setting, and will only work if this key is unset, and, we have a saved value.
    SavedRegistryEntry( HKEY_CURRENT_USER, baseKey.get(), _T(""), 0 ).reset();

    // Notify the system of the changes.
    ::SendMessage( HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Software\\Clients\\StartMenuInternet" );
}

// Restore this entry and corresponding DDE entry.
nsresult ProtocolRegistryEntry::reset() {
    // Restore this entry.
    nsresult rv = SavedRegistryEntry::reset();

    // Do same for corresponding DDE entry.
    DDERegistryEntry( protocol.get() ).reset();

    // For http:, on WindowsXP, we need to do some extra cleanup.
    if ( protocol == RE_LITERAL_STRING( "http" ) ) {
        resetWindowsXP();
    }

    return rv;
}

static DWORD deleteKey( HKEY baseKey, LPCTSTR keyName ) {
    // Make sure input subkey isn't null.
    DWORD rc;
    if ( keyName && _tcslen(keyName) ) {
        // Open subkey.
        HKEY key;
        rc = ::RegOpenKeyEx( baseKey,
                             keyName,
                             0,
                             KEY_ENUMERATE_SUB_KEYS | DELETE,
                             &key );
        // Continue till we get an error or are done.
        while ( rc == ERROR_SUCCESS ) {
            TCHAR subkeyName[_MAX_PATH];
            DWORD len = sizeof subkeyName / sizeof(TCHAR);
            // Get first subkey name.  Note that we always get the
            // first one, then delete it.  So we need to get
            // the first one next time, also.
            rc = ::RegEnumKeyEx( key,
                                 0,
                                 subkeyName,
                                 &len,
                                 0,
                                 0,
                                 0,
                                 0 );
            if ( rc == ERROR_NO_MORE_ITEMS ) {
                // No more subkeys.  Delete the main one.
                rc = ::RegDeleteKey( baseKey, keyName );
                break;
            } else if ( rc == ERROR_SUCCESS ) {
                // Another subkey, delete it, recursively.
                rc = deleteKey( key, subkeyName );
            }
        }
        // Close the key we opened.
        ::RegCloseKey( key );
    } else {
        rc = ERROR_BADKEY;
    }
    return rc;
}

// Set the "dde" entry by deleting the main ddexec subkey
// under HKLM\Software\Classes\<protocol>\shell\open.
// We "set" the various subkeys in order to preserve useful
// information.
nsresult DDERegistryEntry::set() {
    nsresult rv = SavedRegistryEntry::set();
    rv = activate.set();
    rv = app.set();
    rv = topic.set();
    // We've saved what we can.  Now recurse through this key and
    // subkeys.  This is necessary due to the fact that
    // ::RegDeleteKey won't work on WinNT (and Win2k?) if there are
    // subkeys.
    if ( deleteKey( baseKey, keyName.get() ) != ERROR_SUCCESS ) {
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}

// Reset the main (ddeexec) value but also the Application and Topic.
// We reset the app/topic even though we no longer set them.  This
// handles cases where the user installed a prior version, and then
// upgraded.
nsresult DDERegistryEntry::reset() {
    nsresult rv = SavedRegistryEntry::reset();
    rv = activate.reset();
    rv = app.reset();
    rv = topic.reset();
    return rv;
}

// Return current setting for this registry entry.
// Optionally, the caller can ask that a boolean be set to indicate whether
// the registry value is undefined.  This flag can be used to distinguish
// between not defined at all versus simply empty (both of which return an
// empty string).
reString RegistryEntry::currentSetting( PRBool *currentlyUndefined ) const {
    reString result;

    if ( currentlyUndefined ) {
        *currentlyUndefined = PR_TRUE;
    }

    HKEY   key;
    LONG   rc = ::RegOpenKey( baseKey, keyName.get(), &key );
    if ( rc == ERROR_SUCCESS ) {
        TCHAR buffer[4096] = { 0 };
        DWORD len = sizeof buffer;
        rc = ::RegQueryValueEx( key, valueNameArg(), NULL, NULL, (LPBYTE)buffer, &len );
        if ( rc == ERROR_SUCCESS ) {
            result = buffer;
            if ( currentlyUndefined ) {
                *currentlyUndefined = PR_FALSE; // Indicate entry is present.
            }
        }
        ::RegCloseKey( key );
    }

    return result;
}

// For each file extension, map it to this entry's file type.
// Set the file type so this application opens files of that type.
nsresult FileTypeRegistryEntry::set() {
    nsresult rv = NS_OK;

    // Set file extensions.
    for ( int i = 0; NS_SUCCEEDED( rv ) && ext[i]; i++ ) {
        reAutoString thisExt( _T("Software\\Classes\\") );
        thisExt += ext[i];
        rv = SavedRegistryEntry( HKEY_LOCAL_MACHINE, thisExt.get(), _T(""), fileType.get() ).set();
    }

    // If OK, set file type opener.
    if ( NS_SUCCEEDED( rv ) ) {
        rv = ProtocolRegistryEntry::set();

        // If we just created this file type entry, set description and default icon.
        if ( NS_SUCCEEDED( rv ) ) {
            reAutoString descKey( _T("Software\\Classes\\") );
            descKey += protocol;
            RegistryEntry descEntry( HKEY_LOCAL_MACHINE, descKey.get(), NULL, _T("") );
            if ( descEntry.currentSetting().IsEmpty() ) {
                reAutoString defaultDescKey( _T("Software\\Classes\\") );
                defaultDescKey += defDescKey;
                RegistryEntry defaultDescEntry( HKEY_LOCAL_MACHINE, defaultDescKey.get(), NULL, _T("") );

                descEntry.setting = defaultDescEntry.currentSetting();
                if ( descEntry.setting.IsEmpty() )
                    descEntry.setting = desc;
                descEntry.set();
            }
            reAutoString iconKey( _T("Software\\Classes\\") );
            iconKey += protocol;
            iconKey += _T("\\DefaultIcon");

            RegistryEntry iconEntry( HKEY_LOCAL_MACHINE, iconKey.get(), NULL,
                                     reAutoString( thisApplication() + RE_LITERAL_STRING(",0") ).get() );

            if ( iconEntry.currentSetting().IsEmpty() ) {
                iconEntry.set();
            }
        }
    }

    return rv;
}

// Basically, the inverse of set().
// First, reset the opener for this entry's file type.
// Then, reset the file type associated with each extension.
nsresult FileTypeRegistryEntry::reset() {
    nsresult rv = ProtocolRegistryEntry::reset();

    for ( int i = 0; ext[ i ]; i++ ) {
        reAutoString thisExt( _T("Software\\Classes\\") );
        thisExt += ext[i];
        (void)SavedRegistryEntry( HKEY_LOCAL_MACHINE, thisExt.get(), _T(""), fileType.get() ).reset();
    }

    return rv;
}

// Do inherited set() and also set key for edit (with -edit option).
//
// Note: We make the rash assumption that we "own" this filetype (aka "protocol").
// If we ever start commandeering some other file type then this may have to be
// rethought.  The solution is to override reset() and undo this (and make the
// "edit" entry a SavedRegistryEntry).
nsresult EditableFileTypeRegistryEntry::set() {
    nsresult rv = FileTypeRegistryEntry::set();
    if ( NS_SUCCEEDED( rv ) ) {
        reAutoString editKey( _T("Software\\Classes\\") );
        editKey += protocol;
        editKey += _T("\\shell\\edit\\command");
        reAutoString editor( thisApplication() );
        editor += _T(" -edit \"%1\"");
        rv = RegistryEntry( HKEY_LOCAL_MACHINE, editKey.get(), _T(""), editor.get() ).set();
    }
    return rv;
}

// Convert current registry setting to boolean.
BoolRegistryEntry::operator PRBool() {
    return currentSetting().Equals( _T("1") ) ? PR_TRUE : PR_FALSE;
}
