#ifndef __FSpCompat_fopen__
#define __FSpCompat_fopen__

#include <ansi_parms.h>
#include <stdio.h>
#include <Files.h>

#ifdef __cplusplus
extern "C" {
#endif

FILE * FSpCompat_fopen(ConstFSSpecPtr spec, const char * open_mode);

#ifdef __cplusplus
	}
#endif

#endif
