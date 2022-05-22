/*
	app.c
	Copyright (C) 2009 Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "COREDEFS.i"


#define MyMoveBytes(src, dst, n) \
	(void) memmove((void *)(dst), (void *)(src), n)

#define kMyErr_noErr 0 /* no error */

#define kMyErrReported 1029
	/* already notified user, no further report needed */
#define kMyErrNoMatch 1030
	/* (so try something else) always should be handled, not reported */


#include "STRUTILS.i"

#include "CMDARGT1.i"

#include "WRTEXTFL.i"

#include "SPBASDEF.i"

#include "GNBLDOPT.i"
#ifdef Have_SPBLDOPT
#include "SPBLDOPT.i"
#endif

#include "BLDUTIL3.i"

#include "DFFILDEF.i"
#include "SPFILDEF.i"

#include "USFILDEF.i"
#include "WRMACRES.i"
#include "WRMPLIST.i"
#include "WRCNFGGL.i"
#include "WRCNFGAP.i"

#include "WRMPWFLS.i"
#include "WRMW8FLS.i"
#include "WRMVCFLS.i"
#include "WRBGCFLS.i"
#include "WRSNCFLS.i"
#include "WRMSCFLS.i"
#include "WRLCCFLS.i"
#include "WRDVCFLS.i"
#include "WRXCDFLS.i"
#include "WRDMCFLS.i"
#include "WRPLCFLS.i"
#include "WRCCCFLS.i"

#ifdef Have_SPCNFGGL
#include "SPCNFGGL.i"
#endif
#ifdef Have_SPCNFGAP
#include "SPCNFGAP.i"
#endif
#include "SPOTHRCF.i"

#include "BLDUTIL4.i"

int main(int argc, char *argv[])
{
	tMyErr err;
	int return_code = 1;

	BeginParseCommandLineArguments(argc, argv);

	err = DoTheCommand();

	if (kMyErr_noErr == err) {
		return_code = 0;
	} else {
		if (kMyErrReported != err) {
			fprintf(stderr, "Unknown Error in %s", argv[0]);
		}
	}

	return return_code;
}
