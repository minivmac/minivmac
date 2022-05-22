/*
	WRTEXTFL.i
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
	WRite TEXT FiLe
*/



/* --- routines for writing text files --- */


#define WriteCharToOutput putchar

LOCALPROC WriteCStrToOutput(char *s)
{
	printf("%s", s);
}

LOCALPROC WriteSignedLongToOutput(long int v)
{
	printf("%ld", v);
}

LOCALPROC WriteUnsignedToOutput(unsigned int v)
{
	printf("%u", v);
}

LOCALPROC WriteDec2CharToOutput(int v)
{
	printf("%02u", v);
}

LOCALPROC WriteHexByteToOutput(unsigned int v)
{
	printf("%02X", v);
}

LOCALPROC WriteHexWordToOutput(unsigned int v)
{
	printf("%04X", v);
}

LOCALPROC WriteHexLongToOutput(ui5r v)
{
	printf("%08lX", v);
}

LOCALPROC WriteEolToOutput(void)
{
	printf("\n");
}

LOCALPROC WriteLnCStrToOutput(char *s)
{
	WriteCStrToOutput(s);
	WriteEolToOutput();
}


/* --- code specific to Scripting Language --- */

enum {
	gbk_script_mpw, /* Macintosh Programmers Workshop */
	gbk_script_aps, /* AppleScript */
	gbk_script_bsh, /* bash */
	gbk_script_vbs, /* VBScript */
	gbk_script_xps, /* XP */
	kNumScripts
};

LOCALVAR int cur_script;

#ifndef BashUsePrintf
#define BashUsePrintf 1
#endif

#ifndef MPWOneEchoPerFile
#define MPWOneEchoPerFile 0
#endif

GLOBALPROC WriteScriptLangHeader(void)
{
	switch (cur_script) {
		case gbk_script_mpw:
			break;
		case gbk_script_aps:
			break;
		case gbk_script_bsh:
			WriteLnCStrToOutput("#! /bin/bash");
			WriteEolToOutput();
			break;
		case gbk_script_vbs:
			break;
		case gbk_script_xps:
			WriteLnCStrToOutput("@echo off");
			WriteEolToOutput();
			break;
		default:
			break;
	}
}

GLOBALPROC WriteSectionCommentDestFile(char * Description)
{
	WriteEolToOutput();
	WriteEolToOutput();

	switch (cur_script) {
		case gbk_script_mpw:
		case gbk_script_bsh:
			WriteCStrToOutput("# ----- ");
			WriteCStrToOutput(Description);
			WriteCStrToOutput(" -----");
			break;
		case gbk_script_aps:
			WriteCStrToOutput("\t--- ");
			WriteCStrToOutput(Description);
			WriteCStrToOutput(" -----");
			break;
		case gbk_script_vbs:
			WriteCStrToOutput("' ----- ");
			WriteCStrToOutput(Description);
			WriteCStrToOutput(" -----");
			break;
		case gbk_script_xps:
			WriteCStrToOutput("rem ----- ");
			WriteCStrToOutput(Description);
			WriteCStrToOutput(" -----");
			break;
	}

	WriteEolToOutput();
}

LOCALPROC WriteOpenDestFile(char *DirVar, char *FileName, char *FileExt,
	char * Description)
{
	if (nullpr != Description) {
		WriteSectionCommentDestFile(Description);
	}

	WriteEolToOutput();

	switch (cur_script) {
		case gbk_script_mpw:
#if MPWOneEchoPerFile
			WriteCStrToOutput("Echo -n > \"{");
			WriteCStrToOutput(DirVar);
			WriteCStrToOutput("}");
			WriteCStrToOutput(FileName);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput("\" \266");
			WriteEolToOutput();
#else
			WriteCStrToOutput("Set DestFile \"{");
			WriteCStrToOutput(DirVar);
			WriteCStrToOutput("}");
			WriteCStrToOutput(FileName);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput("\"");
			WriteEolToOutput();
			WriteLnCStrToOutput("Echo -n > \"{DestFile}\"");
			WriteEolToOutput();
#endif
			break;
		case gbk_script_aps:
			WriteCStrToOutput(
				"\tset DestFile to open for access file (");
			WriteCStrToOutput(DirVar);
			WriteCStrToOutput(" & \"");
			WriteCStrToOutput(FileName);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput("\") with write permission");
			WriteEolToOutput();
			WriteEolToOutput();
			WriteLnCStrToOutput("\tset eof DestFile to 0");
			break;
		case gbk_script_bsh:
			WriteCStrToOutput("DestFile=\"${");
			WriteCStrToOutput(DirVar);
			WriteCStrToOutput("}");
			WriteCStrToOutput(FileName);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput("\"");
			WriteEolToOutput();
#if BashUsePrintf
			WriteLnCStrToOutput("printf \"\" > \"${DestFile}\"");
#else
			/* WriteLnCStrToOutput("echo -n > \"${DestFile}\""); */
			WriteLnCStrToOutput("true > \"${DestFile}\"");
#endif
			WriteEolToOutput();
			break;
		case gbk_script_vbs:
			WriteCStrToOutput("Set f = fso.CreateTextFile(");
			WriteCStrToOutput(DirVar);
			WriteCStrToOutput(" & \"\\");
			WriteCStrToOutput(FileName);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput("\", True)");
			WriteEolToOutput();
			WriteEolToOutput();
			break;
		case gbk_script_xps:
			WriteCStrToOutput("set DestFile=%");
			WriteCStrToOutput(DirVar);
			WriteCStrToOutput("%");
			WriteCStrToOutput("\\");
			WriteCStrToOutput(FileName);
			WriteCStrToOutput(FileExt);
			WriteEolToOutput();
			WriteLnCStrToOutput("echo.>\"%DestFile%\"");
			WriteLnCStrToOutput("del \"%DestFile%\"");
			break;
	}
}

LOCALPROC WriteCloseDestFile(void)
{
	switch (cur_script) {
		case gbk_script_mpw:
#if MPWOneEchoPerFile
			WriteLnCStrToOutput("''");
#endif
			break;
		case gbk_script_aps:
			WriteEolToOutput();
			WriteLnCStrToOutput("\tclose access DestFile");
			break;
		case gbk_script_bsh:
			break;
		case gbk_script_vbs:
			WriteEolToOutput();
			WriteLnCStrToOutput("f.Close");
			break;
		case gbk_script_xps:
			break;
	}
}

TYPEDEFPROC (*MyProc)(void);

LOCALPROC WriteADstFile1(char *DirVar,
	char *FileName, char *FileExt, char * Description, MyProc p)
{
	WriteOpenDestFile(DirVar, FileName, FileExt, Description);
	p();
	WriteCloseDestFile();
}

LOCALPROC WriteBlankLineToDestFile(void)
{
	switch (cur_script) {
		case gbk_script_mpw:
#if MPWOneEchoPerFile
			WriteLnCStrToOutput("''\266n\266");
#else
			WriteLnCStrToOutput("Echo '' >> \"{DestFile}\"");
#endif
			break;
		case gbk_script_aps:
			WriteLnCStrToOutput("\twrite \"\" & return to DestFile");
			break;
		case gbk_script_bsh:
#if BashUsePrintf
			WriteLnCStrToOutput("printf \"\\n\" >> \"${DestFile}\"");
#else
			WriteLnCStrToOutput("echo '' >> \"${DestFile}\"");
#endif
			break;
		case gbk_script_vbs:
			WriteLnCStrToOutput("f.WriteLine(\"\")");
			break;
		case gbk_script_xps:
			WriteLnCStrToOutput("echo.>>\"%DestFile%\"");
			break;
	}
}

LOCALVAR int DestFileIndent = 0;

LOCALPROC WriteBgnDestFileLn(void)
{
	int i;

	switch (cur_script) {
		case gbk_script_mpw:
#if MPWOneEchoPerFile
			WriteCStrToOutput("'");
#else
			WriteCStrToOutput("Echo '");
#endif
			break;
		case gbk_script_aps:
			WriteCStrToOutput("\twrite \"");
			break;
		case gbk_script_bsh:
#if BashUsePrintf
			WriteCStrToOutput("printf \"%s\\n\" '");
#else
			WriteCStrToOutput("echo '");
#endif
			break;
		case gbk_script_vbs:
			WriteCStrToOutput("f.WriteLine(\"");
			break;
		case gbk_script_xps:
			WriteCStrToOutput("echo ");
			break;
	}

	for (i = 0; i < DestFileIndent; ++i) {
		WriteCStrToOutput("\t");
	}
}

LOCALPROC WriteEndDestFileLn(void)
{
	switch (cur_script) {
		case gbk_script_mpw:
#if MPWOneEchoPerFile
			WriteCStrToOutput("'\266n\266");
#else
			WriteCStrToOutput("' >> \"{DestFile}\"");
#endif
			break;
		case gbk_script_aps:
			WriteCStrToOutput("\" & return to DestFile");
			break;
		case gbk_script_bsh:
			WriteCStrToOutput("' >> \"${DestFile}\"");
			break;
		case gbk_script_vbs:
			WriteCStrToOutput("\")");
			break;
		case gbk_script_xps:
			WriteCStrToOutput(">>\"%DestFile%\"");
			break;
	}

	WriteEolToOutput();
}

LOCALPROC WriteCharsToDestFile(char *p, uimr n)
{
	simr i;
	char c;

	switch (cur_script) {
		case gbk_script_mpw:
			for (i = n; --i >= 0; ) {
				if ('\'' == (c = *p++)) {
					WriteCStrToOutput("'\266''");
				} else {
					WriteCharToOutput(c);
				}
			}
			break;
		case gbk_script_aps:
			for (i = n; --i >= 0; ) {
				if ('"' == (c = *p++)) {
					WriteCStrToOutput("\\\"");
				} else {
					WriteCharToOutput(c);
				}
			}
			break;
		case gbk_script_bsh:
			for (i = n; --i >= 0; ) {
				if ('\'' == (c = *p++)) {
					WriteCStrToOutput("'\\''");
				} else {
					WriteCharToOutput(c);
				}
			}
			break;
		case gbk_script_vbs:
			for (i = n; --i >= 0; ) {
				if ('"' == (c = *p++)) {
					WriteCStrToOutput("\"\"");
				} else {
					WriteCharToOutput(c);
				}
			}
			break;
		case gbk_script_xps:
			for (i = n; --i >= 0; ) {
				c = *p++;
				switch (c) {
					case '%':
						WriteCStrToOutput("%%");
						break;
					case '^':
					case '<':
					case '>':
					case '|':
					case '"':

						/*
							digit preceeding redirection
							modifies the redirection
						*/
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						WriteCStrToOutput("^");
						WriteCharToOutput(c);
						break;
					default:
						WriteCharToOutput(c);
						break;
				}
			}
			break;
	}
}

LOCALPROC WriteBgnCommentBlock(void)
{
	switch (cur_script) {
		case gbk_script_aps:
			WriteLnCStrToOutput("(*");
			break;
		default:
			break;
	}
}

LOCALPROC WriteEndCommentBlock(void)
{
	switch (cur_script) {
		case gbk_script_aps:
			WriteLnCStrToOutput("*)");
			break;
		default:
			break;
	}
}

LOCALPROC WriteBgnCommentBlockLn(void)
{
	switch (cur_script) {
		case gbk_script_mpw:
		case gbk_script_bsh:
			WriteCStrToOutput("# ");
			break;
		case gbk_script_aps:
			WriteCStrToOutput("\t");
			break;
		case gbk_script_vbs:
			WriteCStrToOutput("' ");
			break;
		case gbk_script_xps:
			WriteCStrToOutput("rem ");
			break;
	}
}

LOCALPROC WriteEndCommentBlockLn(void)
{
	WriteEolToOutput();
}

LOCALPROC WriteCommentBlockLn(char *s)
{
	WriteBgnCommentBlockLn();
	WriteCStrToOutput(s);
	WriteEndCommentBlockLn();
}

LOCALPROC MakeSubDirectory(char *new_d, char *parent_d, char *name,
	char *FileExt)
{
	WriteEolToOutput();

	switch (cur_script) {
		case gbk_script_mpw:
			WriteCStrToOutput("Set ");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput(" \"{");
			WriteCStrToOutput(parent_d);
			WriteCStrToOutput("}");
			WriteCStrToOutput(name);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput(":\"");
			WriteEolToOutput();

			WriteCStrToOutput("IF not \"`exists -q \"{");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput("}\"`\"");
			WriteEolToOutput();

			WriteCStrToOutput("\tNewFolder \"{");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput("}\"");
			WriteEolToOutput();

			WriteLnCStrToOutput("END");
			break;
		case gbk_script_aps:
			WriteCStrToOutput("\tset ");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput(" to ");
			WriteCStrToOutput(parent_d);
			WriteCStrToOutput(" & \"");
			WriteCStrToOutput(name);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput(":\"");
			WriteEolToOutput();

			WriteCStrToOutput("\tmake new folder at alias ");
			WriteCStrToOutput(parent_d);
			WriteCStrToOutput(" with properties {name:\"");
			WriteCStrToOutput(name);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput("\"}");
			WriteEolToOutput();
			break;
		case gbk_script_bsh:
			WriteCStrToOutput(new_d);
			WriteCStrToOutput("=\"${");
			WriteCStrToOutput(parent_d);
			WriteCStrToOutput("}");
			WriteCStrToOutput(name);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput("/\"");
			WriteEolToOutput();

			WriteCStrToOutput("if test ! -d \"${");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput("}\" ; then");
			WriteEolToOutput();

			WriteCStrToOutput("\tmkdir \"${");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput("}\"");
			WriteEolToOutput();

			WriteLnCStrToOutput("fi");
			break;
		case gbk_script_vbs:
			WriteCStrToOutput("dim ");
			WriteCStrToOutput(new_d);
			WriteEolToOutput();

			WriteCStrToOutput(new_d);
			WriteCStrToOutput(" = ");
			WriteCStrToOutput(parent_d);
			WriteCStrToOutput(" & \"\\");
			WriteCStrToOutput(name);
			WriteCStrToOutput(FileExt);
			WriteCStrToOutput("\"");
			WriteEolToOutput();

			WriteCStrToOutput("if (NOT fso.FolderExists(");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput(")) Then");
			WriteEolToOutput();

			WriteCStrToOutput("\tfso.CreateFolder(");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput(")");
			WriteEolToOutput();

			WriteLnCStrToOutput("End If");
			break;
		case gbk_script_xps:
			WriteCStrToOutput("set ");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput("=%");
			WriteCStrToOutput(parent_d);
			WriteCStrToOutput("%");
			WriteCStrToOutput("\\");
			WriteCStrToOutput(name);
			WriteCStrToOutput(FileExt);
			WriteEolToOutput();

			WriteCStrToOutput("if not exist \"%");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput("%\" mkdir \"%");
			WriteCStrToOutput(new_d);
			WriteCStrToOutput("%\"");
			WriteEolToOutput();
			break;
	}
}



/* ------- utilities for writing to text files -------- */

LOCALPROC WriteCharToDestFile(char c)
{
	WriteCharsToDestFile(&c, 1);
}

LOCALPROC WriteCStrToDestFile(char *s)
{
	WriteCharsToDestFile(s, CStrLength(s));
}

LOCALPROC WritePStrToDestFile(ps3p s)
{
	MyCharPtr p = s;
	MyCharR n = *p++;

	WriteCharsToDestFile((char *)p, n);
}

LOCALPROC WriteDestFileLn(char *s)
{
	WriteBgnDestFileLn();
	WriteCStrToDestFile(s);
	WriteEndDestFileLn();
}

LOCALPROC WriteSpaceToDestFile(void)
{
	WriteCharToDestFile(' ');
}

LOCALPROC WriteQuoteToDestFile(void)
{
	WriteCharToDestFile('\"');
}

LOCALPROC WriteSingleQuoteToDestFile(void)
{
	WriteCharToDestFile('\'');
}

LOCALPROC WriteBackSlashToDestFile(void)
{
	WriteCharToDestFile('\\');
}

LOCALPROC WriteNUimrToDestFile(uimr v, ui3r n)
{
	MyPStr s;

	PStrFromNUimr(v, n, s);
	WritePStrToDestFile(s);
}
