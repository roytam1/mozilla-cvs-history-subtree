#include "xp_error.h"

#ifdef XP_WIN

#include "assert.h"
#include "xp_mcom.h"
#include "xp_str.h"

#define NOT_NULL(X)	X
#ifdef XP_ASSERT
#undef XP_ASSERT
#endif

#define XP_ASSERT(X) assert(X)
#define LINEBREAK "\n"
#endif

/** compatibility code **/

/* Override the macro definition in xpassert.h if necessary. */
#ifdef XP_AssertAtLine
#undef XP_AssertAtLine
#endif

void XP_AssertAtLine(char *pFileName, int iLine)
{
	assert(0);
}


/** compatibility code from xp_error.c **/


PUBLIC int xp_errno = 0;

/* Get rid of macro definition! */
#undef XP_GetError
#undef XP_SetError

/*
** Provide a procedural implementation in case people mess up their makefiles
*/
int XP_GetError()
{
	return xp_errno;
}


void XP_SetError(int v)
{
    xp_errno = v;
}


/************* secmoz *******************/

/*
 * Password obfuscation, etc.
 */

char *
SECNAV_MungeString(const char *unmunged_string)
{
    return XP_STRDUP(unmunged_string);

}

char *
SECNAV_UnMungeString(const char *munged_string)
{
    return XP_STRDUP(munged_string);
}

