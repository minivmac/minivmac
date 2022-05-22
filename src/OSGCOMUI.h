/*
	OSGCOMUI.h

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
	Operating System Glue COMmon includes, User options Independent

	First include of OSGLUxxx files. Things in common that are not
		affected by user selected options.
		(but are affected by developer selected options)

	May be worthwhile to create a pre-compiled header from this,
		if supported by compiler, which can be used for all
		Variations of a given port.
*/

#ifdef OSGCOMUI_H
#error "header already included"
#else
#define OSGCOMUI_H
#endif


#include "CNFUIOSG.h"
	/*
		Configuration file independent of user options
		for operating system glue.
		Such as includes of API header files
	*/
#include "CNFUIALL.h"
	/*
		Configuration file independent of user options
		for all code.
		In particular, configuration for current compiler.
	*/
#include "DFCNFCMP.h"
	/*
		Default configuration of compiler
		If options for compiler haven't been defined in any
		configuration files, they are defined here.
	*/
#include "ENDIANAC.h"
