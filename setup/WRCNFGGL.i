/*
	WRCNFGGL.i
	Copyright (C) 2007 Paul C. Pratt

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
	WRite "CNFGGLob.h"
		(renamed to "CNFUIALL.h")
*/


LOCALPROC WriteConfigurationWarning(void)
{
	WriteBlankLineToDestFile();
	WriteDestFileLn(
		"This file is automatically generated by the build system,");
	WriteDestFileLn(
		"which tries to know what options are valid in what");
	WriteDestFileLn(
		"combinations. Avoid changing this file manually unless");
	WriteDestFileLn(
		"you know what you're doing.");
}

LOCALPROC WriteCommonCNFUIALLContents(void)
{
	WriteDestFileLn("/*");
	++DestFileIndent;
		WriteDestFileLn(
			"see comment in OSGCOMUI.h");
		WriteConfigurationWarning();
	--DestFileIndent;
	WriteDestFileLn("*/");

	WriteBlankLineToDestFile();
	WriteDestFileLn("/* adapt to current compiler/host processor */");

	if (gbk_ide_mw8 == cur_ide) {
		WriteDestFileLn("/* make sure this is correct CNFUIALL */");

		WriteCheckPreDef("__MWERKS__");
		switch (gbo_cpufam) {
			case gbk_cpufam_68k:
				WriteCheckPreDef("__MC68K__");
				if (gbk_targ_mfpu == cur_targ) {
					WriteCheckPreDef("__MC68881__");
				} else {
					WriteCheckPreNDef("__MC68881__");
				}
				break;
			case gbk_cpufam_ppc:
				WriteCheckPreDef("__POWERPC__");
				break;
			case gbk_cpufam_x86:
				WriteCheckPreDef("__INTEL__");
				break;
		}
	} else if ((gbk_ide_bgc == cur_ide)
		|| (gbk_ide_xcd == cur_ide)
		|| (gbk_ide_mvc == cur_ide))
	{
		switch (gbo_cpufam) {
			case gbk_cpufam_x86:
				WriteDestFileLn("#ifdef __x86_64__");
				WriteDestFileLn("#error \"source is configured for"
					" 32 bit compiler\"");
				WriteDestFileLn("#endif");
				break;
			case gbk_cpufam_x64:
				WriteDestFileLn("#ifdef __i386__");
				WriteDestFileLn("#error \"source is configured for"
					" 64 bit compiler\"");
				WriteDestFileLn("#endif");
				break;
		}
	}

	WriteBlankLineToDestFile();

#if NeedIntFormatInfo
	WriteCompCondBool("MostSigByteFirst",
		(gbk_cpufam_68k == gbo_cpufam)
		|| (gbk_cpufam_ppc == gbo_cpufam));
	WriteCompCondBool("LeastSigByteFirst",
		(gbk_cpufam_x86 == gbo_cpufam)
		|| (gbk_cpufam_x64 == gbo_cpufam));
	WriteCompCondBool("TwosCompSigned",
		(gbk_cpufam_68k == gbo_cpufam)
		|| (gbk_cpufam_ppc == gbo_cpufam)
		|| (gbk_cpufam_x86 == gbo_cpufam)
		|| (gbk_cpufam_x64 == gbo_cpufam));
#endif

	if (gbk_cpufam_68k == gbo_cpufam) {
		WriteDestFileLn("#define HaveCPUfamM68K 1");
	}

	if ((gbk_ide_bgc == cur_ide)
		|| (gbk_ide_xcd == cur_ide)
		|| (gbk_ide_mvc == cur_ide)
		|| (gbk_ide_cyg == cur_ide)
		|| (gbk_ide_dkp == cur_ide))
	{
		WriteDestFileLn(
			"#define MayInline inline __attribute__((always_inline))");
	} else
	if (gbk_ide_snc == cur_ide) {
		WriteDestFileLn("#define MayInline inline");
	} else
	if (gbk_ide_mw8 == cur_ide) {
		WriteDestFileLn("#define MayInline __inline__");
	} else
	if (gbk_ide_msv == cur_ide) {
		if (ide_vers >= 6000) {
			WriteDestFileLn("#define MayInline __forceinline");
		} else {
			WriteDestFileLn("#define MayInline __inline");
		}
	} else
	{
		/* WriteDestFileLn("#define MayInline"); */
	}

	if ((gbk_ide_bgc == cur_ide)
		|| (gbk_ide_xcd == cur_ide)
		|| (gbk_ide_mvc == cur_ide)
		|| (gbk_ide_cyg == cur_ide)
		|| (gbk_ide_dkp == cur_ide))
	{
		WriteDestFileLn(
			"#define MayNotInline __attribute__((noinline))");
	} else
	if ((gbk_ide_msv == cur_ide) && (ide_vers >= 7000)) {
		WriteDestFileLn("#define MayNotInline __declspec(noinline)");
	} else
	{
		/* WriteDestFileLn("#define MayNotInline"); */
	}

	if (gbk_ide_mvc == cur_ide) {
		if ((gbk_cpufam_68k == gbo_cpufam)
			|| (gbk_cpufam_ppc == gbo_cpufam))
		{
			WriteDestFileLn("#define BigEndianUnaligned 1");
			WriteDestFileLn("#define LittleEndianUnaligned 0");
		} else if ((gbk_cpufam_x86 == gbo_cpufam)
			|| (gbk_cpufam_x64 == gbo_cpufam))
		{
			WriteDestFileLn("#define BigEndianUnaligned 0");
			WriteDestFileLn("#define LittleEndianUnaligned 1");
		} else {
			WriteDestFileLn("#define BigEndianUnaligned 0");
			WriteDestFileLn("#define LittleEndianUnaligned 0");
		}

		if (gbk_cpufam_x86 == gbo_cpufam) {
			WriteDestFileLn(
				"#define my_reg_call __attribute__ ((regparm(3)))");
		}

		if (gbk_cpufam_x86 == gbo_cpufam) {
			WriteDestFileLn(
				"#define my_osglu_call __attribute__ "
					"((force_align_arg_pointer))");
		}

		WriteDestFileLn("#define my_cond_rare(x) "
			"(__builtin_expect(x, 0))");
		WriteDestFileLn("#define Have_ASR 1");
		if (gbk_cpufam_x64 == gbo_cpufam) {
			WriteDestFileLn("#define HaveUi6Div 1");
		}
		if (gbk_targ_wcar == cur_targ) {
			WriteDestFileLn("#define HaveUi5to6Mul 0");
		}
		if ((gbk_cpufam_x64 == gbo_cpufam)
			|| (gbk_cpufam_ppc == gbo_cpufam)
			|| (gbk_cpufam_arm == gbo_cpufam))
		{
			WriteDestFileLn("#define HaveGlbReg 1");
		}
		WriteDestFileLn(
			"#define my_align_8 __attribute__ ((aligned (8)))");
	} else if (gbk_ide_xcd == cur_ide) {
		if (CurOfficialBin) {
			if (gbk_cpufam_a64 == gbo_cpufam) {
				WriteDestFileLn("#define BigEndianUnaligned 0");
				WriteDestFileLn("#define LittleEndianUnaligned 1");

				WriteDestFileLn("#define my_cond_rare(x) "
					"(__builtin_expect(x, 0))");
				WriteDestFileLn("#define Have_ASR 1");

				WriteDestFileLn("#define HaveUi6Div 1");

				WriteDestFileLn("#define HaveGlbReg 1");

				WriteDestFileLn(
					"#define my_align_8 __attribute__ ((aligned (8)))");
			}
		}
	}

	WriteCompCondBool("SmallGlobals", gbk_cpufam_68k == gbo_cpufam);

	if ((gbk_ide_bgc == cur_ide)
		|| (gbk_ide_xcd == cur_ide)
		|| (gbk_ide_mvc == cur_ide)
		|| (gbk_ide_ccc == cur_ide)
		|| (gbk_ide_dvc == cur_ide)
		|| (gbk_ide_mgw == cur_ide)
		|| (gbk_ide_dmc == cur_ide)
		|| (gbk_ide_lcc == cur_ide)
		|| (gbk_ide_cyg == cur_ide)
		|| (gbk_ide_dkp == cur_ide)
		)
	{
		WriteDestFileLn("#define cIncludeUnused 0");
	} else {
		WriteDestFileLn("#define cIncludeUnused 1");
	}

	if (gbk_ide_lcc == cur_ide) {
		WriteDestFileLn("#define UnusedParam(x)");
	} else {
		WriteDestFileLn("#define UnusedParam(p) (void) p");
	}

	if (gbk_ide_msv == cur_ide) {
		WriteBlankLineToDestFile();
		WriteDestFileLn("/* --- set up compiler options --- */");
		WriteBlankLineToDestFile();
		WriteDestFileLn("/* ignore integer conversion warnings */");
		WriteDestFileLn(
			"#pragma warning(disable : 4244 4761 4018 4245 4024 4305)");
		WriteBlankLineToDestFile();
		WriteDestFileLn("/* ignore unused inline warning */");
		WriteDestFileLn("#pragma warning(disable : 4514 4714)");
#if 0
		WriteBlankLineToDestFile();
		WriteDestFileLn("/* ignore type redefinition warning */");
		WriteDestFileLn("#pragma warning(disable : 4142)");
#endif
		WriteBlankLineToDestFile();
		WriteDestFileLn(
			"/* ignore unary minus operator"
			" applied to unsigned type warning */");
		WriteDestFileLn("#pragma warning(disable : 4146)");

		if (cur_mIIorIIX
			|| (em_cpu_vers >= 2))
		{
			/* C4127: conditional expression is constant */
			/*
				C4701: local variable may have been used without having
				been initialized
			*/
			WriteBlankLineToDestFile();
			WriteDestFileLn("/* more warnings */");
			WriteDestFileLn("#pragma warning(disable : 4127 4701)");
		}

	} else if (gbk_ide_plc == cur_ide) {
		WriteBlankLineToDestFile();
		WriteDestFileLn("#pragma warn(disable: 2135 2137)");
	}

	if (gbk_ide_mw8 == cur_ide) {
		if (gbk_dbg_on != gbo_dbg) {
			WriteBlankLineToDestFile();
			WriteDestFileLn("#ifdef OptForSpeed");
			WriteDestFileLn("#pragma optimize_for_size off");
			WriteDestFileLn("#endif");
		}
	}

	WriteBlankLineToDestFile();
	WriteDestFileLn("/* --- integer types ---- */");

	/*
		define signed and unsigned integer types
		for 8 bits, 16 bits, 32 bits, and so on.

		the computer might not have integer types
		of a given size. in this case should define
		a type of correct size, such as a structure
		type, even if the computer can't directly do
		integer operations on it. then set
		HaveReal?i?b to 0.
	*/

	/* 8 bits */

	/* (ui3b)0 - (ui3b)1 == (ui3b)255 */
	WriteBlankLineToDestFile();
	WriteDestFileLn("typedef unsigned char ui3b;");
	WriteDestFileLn("#define HaveRealui3b 1");

	/* sizeof(si3b) == sizeof(ui3b) */
	WriteBlankLineToDestFile();
	WriteDestFileLn("typedef signed char si3b;");
	WriteDestFileLn("#define HaveRealsi3b 1");

	/* 16 bits */

	/* (ui4b)0 - (ui4b)1 == (ui4b)65535 */
	WriteBlankLineToDestFile();
	WriteDestFileLn("typedef unsigned short ui4b;");
	WriteDestFileLn("#define HaveRealui4b 1");

	/* sizeof(si4b) == sizeof(ui4b) */
	WriteBlankLineToDestFile();
	WriteDestFileLn("typedef short si4b;");
	WriteDestFileLn("#define HaveRealsi4b 1");

	/* 32 bits */

	/* (ui5b)0 - (ui5b)1 == (ui5b)4294967295 */
	WriteBlankLineToDestFile();
	if ((gbk_cpufam_x64 == gbo_cpufam)
		|| (gbk_cpufam_a64 == gbo_cpufam))
	{
		WriteDestFileLn("typedef unsigned int ui5b;");
	} else {
		WriteDestFileLn("typedef unsigned long ui5b;");
	}
	WriteDestFileLn("#define HaveRealui5b 1");

	/* sizeof(si5b) == sizeof(ui5b) */
	WriteBlankLineToDestFile();
	if ((gbk_cpufam_x64 == gbo_cpufam)
		|| (gbk_cpufam_a64 == gbo_cpufam))
	{
		WriteDestFileLn("typedef int si5b;");
	} else {
		WriteDestFileLn("typedef long si5b;");
	}
	WriteDestFileLn("#define HaveRealsi5b 1");

	/* 64 bits */ /* this is mostly for illustration, not used */
#if 0
	struct ui6b {
		ui5b f0;
		ui5b f1;
	};
	typedef struct ui6b ui6b;

	struct si6b {
		ui5b f0;
		si5b f1;
	};
	typedef struct si6b si6b;
#endif
	WriteBlankLineToDestFile();
	WriteDestFileLn("#define HaveRealui6b 0");
	WriteDestFileLn("#define HaveRealsi6b 0");

	WriteBlankLineToDestFile();
	WriteDestFileLn("/* --- integer representation types ---- */");

	/*
		for each integer type, define
		the most efficient representation
		for parameter passing and temporary
		variables on the current
		computer.
	*/

	WriteBlankLineToDestFile();
#if ModPPCi3rTypes
	if (gbk_cpufam_ppc == gbo_cpufam) {
		WriteDestFileLn("typedef ui5b ui3r;");
		WriteDestFileLn("#define ui3beqr 0");
	} else
#endif
	{
		WriteDestFileLn("typedef ui3b ui3r;");
		WriteDestFileLn("#define ui3beqr 1");
	}

	WriteBlankLineToDestFile();
#if ModPPCi3rTypes
	if (gbk_cpufam_ppc == gbo_cpufam) {
		WriteDestFileLn("typedef si5b si3r;");
		WriteDestFileLn("#define si3beqr 0");
	} else
#endif
	{
		WriteDestFileLn("typedef si3b si3r;");
		WriteDestFileLn("#define si3beqr 1");
	}

	WriteBlankLineToDestFile();
	WriteDestFileLn("typedef ui4b ui4r;");
	WriteDestFileLn("#define ui4beqr 1");

	WriteBlankLineToDestFile();
	WriteDestFileLn("typedef si4b si4r;");
	WriteDestFileLn("#define si4beqr 1");

	WriteBlankLineToDestFile();
	WriteDestFileLn("typedef ui5b ui5r;");
	WriteDestFileLn("#define ui5beqr 1");

	WriteBlankLineToDestFile();
	WriteDestFileLn("typedef si5b si5r;");
	WriteDestFileLn("#define si5beqr 1");

	if (gbk_ide_mvc == cur_ide) {
		if (gbk_cpufam_x86 == gbo_cpufam)
		{
			WriteBlankLineToDestFile();
			WriteDestFileLn("/* for probable register parameters */");
			WriteDestFileLn("#define ui4rr ui5r");
			WriteDestFileLn("#define ui3rr ui5r");
		} else if (gbk_cpufam_x64 == gbo_cpufam) {
#if 0
			WriteBlankLineToDestFile();
			WriteDestFileLn("/* for probable register parameters */");
			WriteDestFileLn("#define ui4rr unsigned long int");
			WriteDestFileLn("#define ui3rr unsigned long int");
#endif
			WriteDestFileLn("#define si5rr signed long");
		}

		WriteBlankLineToDestFile();
		WriteDestFileLn(
			"#define MySwapUi5r(x) ((ui5r)__builtin_bswap32(x))");
		WriteDestFileLn("#define HaveMySwapUi5r 1");
	} else if (gbk_ide_xcd == cur_ide) {
		if (CurOfficialBin) {
			if (gbk_cpufam_a64 == gbo_cpufam) {
				WriteBlankLineToDestFile();
				WriteDestFileLn(
					"#define MySwapUi5r(x)"
					" ((ui5r)__builtin_bswap32(x))");
				WriteDestFileLn("#define HaveMySwapUi5r 1");
			}
		}
	}
}

LOCALPROC Write64bitConfig(void)
{
	WriteBlankLineToDestFile();
	if (gbk_ide_msv == cur_ide) {
		WriteDestFileLn("typedef signed __int64 si6r;");
		WriteDestFileLn("typedef signed __int64 si6b;");
		WriteDestFileLn("typedef unsigned __int64 ui6r;");
		WriteDestFileLn("typedef unsigned __int64 ui6b;");
		WriteDestFileLn("#define LIT64(a) a##Ui64");
	} else {
		WriteDestFileLn("typedef signed long long si6r;");
		WriteDestFileLn("typedef signed long long si6b;");
		WriteDestFileLn("typedef unsigned long long ui6r;");
		WriteDestFileLn("typedef unsigned long long ui6b;");
		WriteDestFileLn("#define LIT64(a) a##ULL");
	}
}
