/*
	PICOMMON.h

	Copyright (C) 2020 Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

/*
	Platform Idendependent code COMMON

	First include of platform idendependent code files,
		containing definitions used by all of them.

	May be worthwhile to create a pre-compiled header from this,
		if supported by compiler, which can be used for all
		platform idendependent files of a given Variation.
*/

#ifdef PICOMMON_H
#error "header already included"
#else
#define PICOMMON_H
#endif

#include "CNFUIALL.h"
	/* see OSGCOMUI.h for comment */
#include "CNFUIPIC.h"
	/*
		Configuration file independent of user options
		suitable for platform indendent code.
		Usually empty, but if use different compiler for
		operating system glue, then could define the different compiler
		configuration here and in CNFUIOSG, instead of CNFUIALL.
	*/
#include "DFCNFCMP.h"
	/* see OSGCOMUI.h for comment */
#include "ENDIANAC.h"
#include "CNFUDALL.h"
	/* see OSGCOMUD.h for comment */
#include "OSGLUAAA.h"
#include "CNFUDPIC.h"
	/*
		Configuration file dependent on user options
		suitable for platform indendent code.
	*/
#include "GLOBGLUE.h"
