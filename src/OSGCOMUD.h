/*
	OSGCOMUD.h

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
	Operating System Glue COMmon includes, User options Dependent

	Second include of OSGLUxxx files. All things in common
	that can not go in OSGLCMUI, because they depend on user options.
*/

#ifdef OSGCOMUD_H
#error "header already included"
#else
#define OSGCOMUD_H
#endif

#include "CNFUDOSG.h"
	/*
		Configuration file dependent on user options
		for operating system glue.
	*/
#include "CNFUDALL.h"
	/*
		Configuration file dependent on user options
		for all code.
	*/
#include "OSGLUAAA.h"

#include "STRCONST.h"
