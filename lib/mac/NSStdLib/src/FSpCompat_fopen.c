#include "FSpCompat_fopen.h"
#include <FSp_fopen.h>
#include <string.h>
#include <Script.h>

extern char system_has_new_file_apis(void);

FILE * FSpCompat_fopen(ConstFSSpecPtr spec, const char * open_mode)
{
	// Major drag: If _MSL_USE_NEW_FILE_APIS or _MSL_USE_OLD_AND_NEW_FILE_APIS
	// are defined in ansi_prefix.mac.h (the default), we cannot use FSp_fopen.
	// It opens the file and ends up with an old-fashioned refnum but all of
	// the I/O routines require a fileref. The problem is in the CW file FSp_fopen.c.
	// It should, depending on these flags, open the file in whatever way the
	// I/O routines require.

#ifdef _MSL_USE_OLD_AND_NEW_FILE_APIS
	if (system_has_new_file_apis())
	{
#endif

#ifdef _MSL_USE_NEW_FILE_APIS
		OSErr	err;
		FSRef 	fileref;		
		err = FSpMakeFSRef(spec, &fileref);
		if (err == fnfErr && open_mode[0] == 'w') {
			OSType type = (strchr(open_mode, 'b') != 0) ? 'BINA' : 'TEXT';
			err = FSpCreate(spec, 'CWIE', type, smRoman);
			if (err == noErr)
				err = FSpMakeFSRef(spec, &fileref);
		}
		return (err == noErr ? FSRef_fopen(&fileref, open_mode) : NULL);
#endif

#ifdef _MSL_USE_OLD_AND_NEW_FILE_APIS
	}
	else
	{
#endif

#ifdef _MSL_USE_OLD_FILE_APIS
		return FSp_fopen(spec, open_mode);
#endif

#ifdef _MSL_USE_OLD_AND_NEW_FILE_APIS
	}
#endif

}
