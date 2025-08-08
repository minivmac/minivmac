/*
	OSGLUDOS.c

	Copyright (C) 2009-2024 StevenSYS, Paul C. Pratt, 
	and probably some others

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
	Operating System GLUe for (ms-)DOS
*/

#include "OSGCOMUI.h"
#include "OSGCOMUD.h"
#include "CNFUIPIC.h"

#ifdef WantOSGLUDOS

/* --- some simple utilities --- */

GLOBALOSGLUPROC MyMoveBytes(anyp srcPtr, anyp destPtr, si5b byteCount) {
	(void) memcpy((char *)destPtr, (char *)srcPtr, byteCount);
}

/* --- control mode and internationalization --- */

#define NeedCell2PlainAsciiMap 1

#include "INTLCHAR.h"

/* --- sending debugging info to file --- */

#if dbglog_HAVE

#ifndef dbglog_ToStdErr
#define dbglog_ToStdErr 0
#endif

#if ! dbglog_ToStdErr
LOCALVAR FILE *dbglog_File = NULL;
#endif

LOCALFUNC blnr dbglog_open0(void) {
#if dbglog_ToStdErr
	return trueblnr;
#else
	dbglog_File = fopen("dbglog.txt", "w");
	return (NULL != dbglog_File);
#endif
}

LOCALPROC dbglog_write0(char *s, uimr L) {
#if dbglog_ToStdErr
	(void) fwrite(s, 1, L, stderr);
#else
	if (dbglog_File != NULL) {
		(void) fwrite(s, 1, L, dbglog_File);
	}
#endif
}

LOCALPROC dbglog_close0(void) {
#if ! dbglog_ToStdErr
	if (dbglog_File != NULL) {
		fclose(dbglog_File);
		dbglog_File = NULL;
	}
#endif
}

#endif

/* --- debug settings and utilities --- */

#if ! dbglog_HAVE
#define WriteExtraErr(s)
#else
LOCALPROC WriteExtraErr(char *s) {
	dbglog_writeCStr("*** error: ");
	dbglog_writeCStr(s);
	dbglog_writeReturn();
}
#endif

#define MyDbgEvents (dbglog_HAVE && 0)

#if MyDbgEvents
LOCALPROC WriteDbgAtom(char *s, Atom x) {
	char *name = XGetAtomName(x_display, x);
	if (name != NULL) {
		dbglog_writeCStr("Atom ");
		dbglog_writeCStr(s);
		dbglog_writeCStr(": ");
		dbglog_writeCStr(name);
		dbglog_writeReturn();
		XFree(name);
	}
}
#endif

/* --- information about the environment --- */

#include "COMOSGLU.h"

#include "PBUFSTDC.h"

#include "CONTROLM.h"

/* --- text translation --- */

#if IncludePbufs
/* this is table for Windows, any changes needed for X? */
LOCALVAR const ui3b Native2MacRomanTab[] = {
	0xAD, 0xB0, 0xE2, 0xC4, 0xE3, 0xC9, 0xA0, 0xE0,
	0xF6, 0xE4, 0xB6, 0xDC, 0xCE, 0xB2, 0xB3, 0xB7,
	0xB8, 0xD4, 0xD5, 0xD2, 0xD3, 0xA5, 0xD0, 0xD1,
	0xF7, 0xAA, 0xC5, 0xDD, 0xCF, 0xB9, 0xC3, 0xD9,
	0xCA, 0xC1, 0xA2, 0xA3, 0xDB, 0xB4, 0xBA, 0xA4,
	0xAC, 0xA9, 0xBB, 0xC7, 0xC2, 0xBD, 0xA8, 0xF8,
	0xA1, 0xB1, 0xC6, 0xD7, 0xAB, 0xB5, 0xA6, 0xE1,
	0xFC, 0xDA, 0xBC, 0xC8, 0xDE, 0xDF, 0xF0, 0xC0,
	0xCB, 0xE7, 0xE5, 0xCC, 0x80, 0x81, 0xAE, 0x82,
	0xE9, 0x83, 0xE6, 0xE8, 0xED, 0xEA, 0xEB, 0xEC,
	0xF5, 0x84, 0xF1, 0xEE, 0xEF, 0xCD, 0x85, 0xF9,
	0xAF, 0xF4, 0xF2, 0xF3, 0x86, 0xFA, 0xFB, 0xA7,
	0x88, 0x87, 0x89, 0x8B, 0x8A, 0x8C, 0xBE, 0x8D,
	0x8F, 0x8E, 0x90, 0x91, 0x93, 0x92, 0x94, 0x95,
	0xFD, 0x96, 0x98, 0x97, 0x99, 0x9B, 0x9A, 0xD6,
	0xBF, 0x9D, 0x9C, 0x9E, 0x9F, 0xFE, MKC_None, 0xD8
};
#endif

#if IncludePbufs
LOCALFUNC tMacErr NativeTextToMacRomanPbuf(char *x, tPbuf *r) {
	if (NULL == x) {
		return mnvm_miscErr;
	} else {
		ui3p p;
		ui5b L = strlen(x);

		p = (ui3p)malloc(L);
		if (NULL == p) {
			return mnvm_miscErr;
		} else {
			ui3b *p0 = (ui3b *)x;
			ui3b *p1 = (ui3b *)p;
			int i;

			for (i = L; --i >= 0; ) {
				ui3b v = *p0++;
				if (v >= 128) {
					v = Native2MacRomanTab[v - 128];
				} else if (10 == v) {
					v = 13;
				}
				*p1++ = v;
			}

			return PbufNewFromPtr(p, L, r);
		}
	}
}
#endif

#if IncludePbufs
/* this is table for Windows, any changes needed for X? */
LOCALVAR const ui3b MacRoman2NativeTab[] = {
	0xC4, 0xC5, 0xC7, 0xC9, 0xD1, 0xD6, 0xDC, 0xE1,
	0xE0, 0xE2, 0xE4, 0xE3, 0xE5, 0xE7, 0xE9, 0xE8,
	0xEA, 0xEB, 0xED, 0xEC, 0xEE, 0xEF, 0xF1, 0xF3,
	0xF2, 0xF4, 0xF6, 0xF5, 0xFA, 0xF9, 0xFB, 0xFC,
	0x86, 0xB0, 0xA2, 0xA3, 0xA7, 0x95, 0xB6, 0xDF,
	0xAE, 0xA9, 0x99, 0xB4, 0xA8, 0x80, 0xC6, 0xD8,
	0x81, 0xB1, 0x8D, 0x8E, 0xA5, 0xB5, 0x8A, 0x8F,
	0x90, 0x9D, 0xA6, 0xAA, 0xBA, 0xAD, 0xE6, 0xF8,
	0xBF, 0xA1, 0xAC, 0x9E, 0x83, 0x9A, 0xB2, 0xAB,
	0xBB, 0x85, 0xA0, 0xC0, 0xC3, 0xD5, 0x8C, 0x9C,
	0x96, 0x97, 0x93, 0x94, 0x91, 0x92, 0xF7, 0xB3,
	MKC_None, 0x9F, 0xB9, 0xA4, 0x8B, 0x9B, 0xBC, 0xBD,
	0x87, 0xB7, 0x82, 0x84, 0x89, 0xC2, 0xCA, 0xC1,
	0xCB, 0xC8, 0xCD, 0xCE, 0xCF, 0xCC, 0xD3, 0xD4,
	0xBE, 0xD2, 0xDA, 0xDB, 0xD9, 0xD0, 0x88, 0x98,
	0xAF, 0xD7, 0xDD, 0xDE, 0xB8, 0xF0, 0xFD, 0xFE
};
#endif

#if IncludePbufs
LOCALFUNC blnr MacRomanTextToNativePtr(tPbuf i, blnr IsFileName,
	ui3p *r) {
	ui3p p;
	void *Buffer = PbufDat[i];
	ui5b L = PbufSize[i];

	p = (ui3p)malloc(L + 1);
	if (p != NULL) {
		ui3b *p0 = (ui3b *)Buffer;
		ui3b *p1 = (ui3b *)p;
		int j;

		if (IsFileName) {
			for (j = L; --j >= 0; ) {
				ui3b x = *p0++;
				if (x < 32) {
					x = '-';
				} else if (x >= 128) {
					x = MacRoman2NativeTab[x - 128];
				} else {
					switch (x) {
						case '/':
						case '<':
						case '>':
						case '|':
						case ':':
							x = '-';
						default:
							break;
					}
				}
				*p1++ = x;
			}
			if ('.' == p[0]) {
				p[0] = '-';
			}
		} else {
			for (j = L; --j >= 0; ) {
				ui3b x = *p0++;
				if (x >= 128) {
					x = MacRoman2NativeTab[x - 128];
				} else if (13 == x) {
					x = '\n';
				}
				*p1++ = x;
			}
		}
		*p1 = 0;

		*r = p;
		return trueblnr;
	}
	return falseblnr;
}
#endif

LOCALPROC NativeStrFromCStr(char *r, char *s) {
	ui3b ps[ClStrMaxLength];
	int i;
	int L;

	ClStrFromSubstCStr(&L, ps, s);

	for (i = 0; i < L; ++i) {
		r[i] = Cell2PlainAsciiMap[ps[i]];
	}

	r[L] = 0;
}


/* --- drives --- */

#define NotAfileRef NULL

LOCALVAR FILE *Drives[NumDrives]; /* open disk image files */
#if IncludeSonyGetName || IncludeSonyNew
LOCALVAR char *DriveNames[NumDrives];
#endif

GLOBALOSGLUFUNC tMacErr vSonyTransfer(blnr IsWrite, ui3p Buffer,
	tDrive Drive_No, ui5r Sony_Start, ui5r Sony_Count,
	ui5r *Sony_ActCount) {
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	ui5r NewSony_Count = 0;

	if (0 == fseek(refnum, Sony_Start, SEEK_SET)) {
		if (IsWrite) {
			NewSony_Count = fwrite(Buffer, 1, Sony_Count, refnum);
		} else {
			NewSony_Count = fread(Buffer, 1, Sony_Count, refnum);
		}

		if (NewSony_Count == Sony_Count) {
			err = mnvm_noErr;
		}
	}

	if (nullpr != Sony_ActCount) {
		*Sony_ActCount = NewSony_Count;
	}

	return err; /*& figure out what really to return &*/
}

GLOBALOSGLUFUNC tMacErr vSonyGetSize(tDrive Drive_No, ui5r *Sony_Count) {
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	long v;

	if (0 == fseek(refnum, 0, SEEK_END)) {
		v = ftell(refnum);
		if (v >= 0) {
			*Sony_Count = v;
			err = mnvm_noErr;
		}
	}

	return err; /*& figure out what really to return &*/
}

LOCALFUNC tMacErr vSonyEjeccurrentPixel(tDrive Drive_No, blnr deleteit) {
	FILE *refnum = Drives[Drive_No];

	DiskEjectedNotify(Drive_No);

	fclose(refnum);
	Drives[Drive_No] = NotAfileRef; /* not really needed */

#if IncludeSonyGetName || IncludeSonyNew
	{
		char *s = DriveNames[Drive_No];
		if (NULL != s) {
			if (deleteit) {
				remove(s);
			}
			free(s);
			DriveNames[Drive_No] = NULL; /* not really needed */
		}
	}
#endif

	return mnvm_noErr;
}

GLOBALOSGLUFUNC tMacErr vSonyEject(tDrive Drive_No) {
	return vSonyEjeccurrentPixel(Drive_No, falseblnr);
}

#if IncludeSonyNew
GLOBALOSGLUFUNC tMacErr vSonyEjectDelete(tDrive Drive_No) {
	return vSonyEjeccurrentPixel(Drive_No, trueblnr);
}
#endif

LOCALPROC UnInitDrives(void) {
	tDrive i;

	for (i = 0; i < NumDrives; ++i) {
		if (vSonyIsInserted(i)) {
			(void) vSonyEject(i);
		}
	}
}

#if IncludeSonyGetName
GLOBALOSGLUFUNC tMacErr vSonyGetName(tDrive Drive_No, tPbuf *r) {
	char *drivepath = DriveNames[Drive_No];
	if (NULL == drivepath) {
		return mnvm_miscErr;
	} else {
		char *s = strrchr(drivepath, '/');
		if (NULL == s) {
			s = drivepath;
		} else {
			++s;
		}
		return NativeTextToMacRomanPbuf(s, r);
	}
}
#endif

LOCALFUNC blnr Sony_InsercurrentPixel(FILE *refnum, blnr locked,
	char *drivepath) {
	tDrive Drive_No;
	blnr IsOk = falseblnr;

	if (! FirstFreeDisk(&Drive_No)) {
		MacMsg(kStrTooManyImagesTitle, kStrTooManyImagesMessage,
			falseblnr);
	} else {
		/* printf("Sony_InsercurrentPixel %d\n", (int)Drive_No); */

		{
			Drives[Drive_No] = refnum;
			DiskInsertNotify(Drive_No, locked);

#if IncludeSonyGetName || IncludeSonyNew
			{
				ui5b L = strlen(drivepath);
				char *p = malloc(L + 1);
				if (p != NULL) {
					(void) memcpy(p, drivepath, L + 1);
				}
				DriveNames[Drive_No] = p;
			}
#endif

			IsOk = trueblnr;
		}
	}

	if (! IsOk) {
		fclose(refnum);
	}

	return IsOk;
}

LOCALFUNC blnr Sony_Insert1(char *drivepath, blnr silentfail) {
	blnr locked = falseblnr;
	/* printf("Sony_Insert1 %s\n", drivepath); */
	FILE *refnum = fopen(drivepath, "rb+");
	if (NULL == refnum) {
		locked = trueblnr;
		refnum = fopen(drivepath, "rb");
	}
	if (NULL == refnum) {
		if (! silentfail) {
			MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
		}
	} else {
		return Sony_InsercurrentPixel(refnum, locked, drivepath);
	}

	return falseblnr;
}

#define Sony_Insert2(s) Sony_Insert1(s, trueblnr)

LOCALFUNC blnr Sony_InsertIth(int i) {
	blnr v;

	if ((i > 9) || ! FirstFreeDisk(nullpr)) {
		v = falseblnr;
	} else {
		char s[] = "disk?.dsk";

		s[4] = '0' + i;

		v = Sony_Insert2(s);
	}

	return v;
}

LOCALFUNC blnr LoadInitialImages(void) {
	int i;

	for (i = 1; Sony_InsertIth(i); ++i) {
		/* stop on first error (including file not found) */
	}

	return trueblnr;
}

#if IncludeSonyNew
LOCALFUNC blnr WriteZero(FILE *refnum, ui5b L) {
#define ZeroBufferSize 2048
	ui5b i;
	ui3b buffer[ZeroBufferSize];

	memset(&buffer, 0, ZeroBufferSize);

	while (L > 0) {
		i = (L > ZeroBufferSize) ? ZeroBufferSize : L;
		if (fwrite(buffer, 1, i, refnum) != i) {
			return falseblnr;
		}
		L -= i;
	}
	return trueblnr;
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDisk(ui5b L, char *drivepath) {
	blnr IsOk = falseblnr;
	FILE *refnum = fopen(drivepath, "wb+");
	if (NULL == refnum) {
		MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
	} else {
		if (WriteZero(refnum, L)) {
			IsOk = Sony_InsercurrentPixel(refnum, falseblnr, drivepath);
			refnum = NULL;
		}
		if (refnum != NULL) {
			fclose(refnum);
		}
		if (! IsOk) {
			(void) remove(drivepath);
		}
	}
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDiskAtDefault(ui5b L) {
	char s[ClStrMaxLength + 1];

	NativeStrFromCStr(s, "untitled.dsk");
	MakeNewDisk(L, s);
}
#endif

/* --- ROM --- */

LOCALVAR char *rom_path = NULL;

LOCALFUNC tMacErr LoadMacRomFrom(char *path) {
	tMacErr err;
	FILE *ROM_File;
	int File_Size;

	ROM_File = fopen(path, "rb");
	if (NULL == ROM_File) {
		err = mnvm_fnfErr;
	} else {
		File_Size = fread(ROM, 1, kROM_Size, ROM_File);
		if (kROM_Size != File_Size) {
			if (feof(ROM_File)) {
				MacMsgOverride(kStrShortROMTitle,
					kStrShortROMMessage);
				err = mnvm_eofErr;
			} else {
				MacMsgOverride(kStrNoReadROMTitle,
					kStrNoReadROMMessage);
				err = mnvm_miscErr;
			}
		} else {
			err = ROM_IsValid();
		}
		fclose(ROM_File);
	}

	return err;
}

LOCALFUNC blnr LoadMacRom(void) {
	tMacErr err;

	if (mnvm_fnfErr == (err = LoadMacRomFrom(RomFileName))) {
	}

	return trueblnr; /* keep launching Mini vMac, regardless */
}

/* --- command line parsing --- */
LOCALVAR int my_argc;
LOCALVAR char **my_argv;
LOCALVAR int mouseEdgeStick = 0;

LOCALFUNC blnr ScanCommandLine(void) {
	int i;

	for (i = 1; i < my_argc; i++) {
		if ('-' == my_argv[i][0]) {
			if ((0 == strcmp(my_argv[i], "--rom")) || (0 == strcmp(my_argv[i], "-r"))) {
				++i;
				if (i < my_argc) {
					rom_path = my_argv[i];
				}
			} else if ((0 == strcmp(my_argv[i], "--nomousestick")) || (0 == strcmp(my_argv[i], "-n"))) {
				mouseEdgeStick = 1;
			} else {
				MacMsg(kStrBadArgTitle, kStrBadArgMessage, falseblnr);
			}
		} else {
			(void) Sony_Insert1(my_argv[i], falseblnr);
		}
	}
	return trueblnr;
}


/* --- video out --- */

/* VESA_CopyToScreen(FrameBuffer, sizeof(FrameBuffer)); */

LOCALVAR blnr CurSpeedStopped = trueblnr;
LOCALVAR blnr FrameBuffer[VESAWidth*VESAHeight];
int renderOffset[2] = { (VESAWidth - vMacScreenWidth) / 2, (VESAHeight - vMacScreenHeight) / 2};

/* https://www.delorie.com/djgpp/doc/ug/graphics/vesa.html.en */
typedef struct VESA_INFO { 
	char	VESASignature[4]; 
	unsigned short	VESAVersion;
	unsigned long	OEMStringPtr;
	unsigned char	Capabilities[4];
	unsigned long	VideoModePtr;
	unsigned short	TotalMemory;
	unsigned short	OemSoftwareRev;
	unsigned long	OemVendorNamePtr;
	unsigned long	OemProductNamePt;
	unsigned long	OemProductRevPtr;
	unsigned char	Reserved[222];
	unsigned char	OemData[256];
} VESA_INFO;

typedef struct MODE_INFO {
	unsigned short	ModeAttributes;
	unsigned char	WinAAttributes;
	unsigned char	WinBAttributes;
	unsigned short	WinGranularity;
	unsigned short	WinSize;
	unsigned short	WinASegment;
	unsigned short	WinBSegment;
	unsigned long	WinFuncPtr;
	unsigned short	BytesPerScanLine;
	unsigned short	XResolution;
	unsigned short	YResolution;
	unsigned char	XCharSize;
	unsigned char	YCharSize;
	unsigned char	NumberOfPlanes;
	unsigned char	BitsPerPixel;
	unsigned char	NumberOfBanks;
	unsigned char	MemoryModel;
	unsigned char	BankSize;
	unsigned char	NumberOfImagePages;
	unsigned char	Reserved_page;
	unsigned char	RedMaskSize;
	unsigned char	RedMaskPos;
	unsigned char	GreenMaskSize;
	unsigned char	GreenMaskPos;
	unsigned char	BlueMaskSize;
	unsigned char	BlueMaskPos;
	unsigned char	ReservedMaskSize;
	unsigned char	ReservedMaskPos;
	unsigned char	DirectColorModeInfo;
	unsigned long	PhysBasePtr;
	unsigned long	OffScreenMemOffset;
	unsigned short	OffScreenMemSize;
	unsigned char	Reserved[206];
} MODE_INFO;

#if vMacScreenDepth > 0
int video_colorPalette[256][3] = {
	{ 0,0,0 },
	{ 0,0,0 },
	{ 8,8,8 },
	{ 32,32,32 },
	{ 65,68,65 },
	{ 82,85,82 },
	{ 115,117,115 },
	{ 139,137,139 },
	{ 172,170,172 },
	{ 189,186,189 },
	{ 222,222,222 },
	{ 238,238,238 },
	{ 0,0,8 },
	{ 0,0,32 },
	{ 0,0,65 },
	{ 0,0,82 },
	{ 0,0,115 },
	{ 0,0,139 },
	{ 0,0,172 },
	{ 0,0,189 },
	{ 0,0,222 },
	{ 0,0,238 },
	{ 0,8,0 },
	{ 0,32,0 },
	{ 0,68,0 },
	{ 0,85,0 },
	{ 0,117,0 },
	{ 0,137,0 },
	{ 0,170,0 },
	{ 0,186,0 },
	{ 0,222,0 },
	{ 0,238,0 },
	{ 8,0,0 },
	{ 49,0,0 },
	{ 65,0,0 },
	{ 82,0,0 },
	{ 115,0,0 },
	{ 139,0,0 },
	{ 172,0,0 },
	{ 189,0,0 },
	{ 222,0,0 },
	{ 238,0,0 },
	{ 0,0,49 },
	{ 0,0,98 },
	{ 0,0,156 },
	{ 0,0,205 },
	{ 0,0,255 },
	{ 0,48,0 },
	{ 0,48,49 },
	{ 0,48,98 },
	{ 0,48,156 },
	{ 0,48,205 },
	{ 0,48,255 },
	{ 0,101,0 },
	{ 0,101,49 },
	{ 0,101,98 },
	{ 0,101,156 },
	{ 0,101,205 },
	{ 0,101,255 },
	{ 0,153,0 },
	{ 0,153,49 },
	{ 0,153,98 },
	{ 0,153,156 },
	{ 0,153,205 },
	{ 0,153,255 },
	{ 0,206,0 },
	{ 0,206,49 },
	{ 0,206,98 },
	{ 0,206,156 },
	{ 0,206,205 },
	{ 0,206,255 },
	{ 0,255,0 },
	{ 0,255,49 },
	{ 0,255,98 },
	{ 0,255,156 },
	{ 0,255,205 },
	{ 0,255,255 },
	{ 49,0,49 },
	{ 49,0,98 },
	{ 49,0,156 },
	{ 49,0,205 },
	{ 49,0,255 },
	{ 49,48,0 },
	{ 49,48,49 },
	{ 49,48,98 },
	{ 49,48,156 },
	{ 49,48,205 },
	{ 49,48,255 },
	{ 49,101,0 },
	{ 49,101,49 },
	{ 49,101,98 },
	{ 49,101,156 },
	{ 49,101,205 },
	{ 49,101,255 },
	{ 49,153,0 },
	{ 49,153,49 },
	{ 49,153,98 },
	{ 49,153,156 },
	{ 49,153,205 },
	{ 49,153,255 },
	{ 49,206,0 },
	{ 49,206,49 },
	{ 49,206,98 },
	{ 49,206,156 },
	{ 49,206,205 },
	{ 49,206,255 },
	{ 49,255,0 },
	{ 49,255,49 },
	{ 49,255,98 },
	{ 49,255,156 },
	{ 49,255,205 },
	{ 49,255,255 },
	{ 98,0,0 },
	{ 98,0,49 },
	{ 98,0,98 },
	{ 98,0,156 },
	{ 98,0,205 },
	{ 98,0,255 },
	{ 98,48,0 },
	{ 98,48,49 },
	{ 98,48,98 },
	{ 98,48,156 },
	{ 98,48,205 },
	{ 98,48,255 },
	{ 98,101,0 },
	{ 98,101,49 },
	{ 98,101,98 },
	{ 98,101,156 },
	{ 98,101,205 },
	{ 98,101,255 },
	{ 98,153,0 },
	{ 98,153,49 },
	{ 98,153,98 },
	{ 98,153,156 },
	{ 98,153,205 },
	{ 98,153,255 },
	{ 98,206,0 },
	{ 98,206,49 },
	{ 98,206,98 },
	{ 98,206,156 },
	{ 98,206,205 },
	{ 98,206,255 },
	{ 98,255,0 },
	{ 98,255,49 },
	{ 98,255,98 },
	{ 98,255,156 },
	{ 98,255,205 },
	{ 98,255,255 },
	{ 156,0,0 },
	{ 156,0,49 },
	{ 156,0,98 },
	{ 156,0,156 },
	{ 156,0,205 },
	{ 156,0,255 },
	{ 156,48,0 },
	{ 156,48,49 },
	{ 156,48,98 },
	{ 156,48,156 },
	{ 156,48,205 },
	{ 156,48,255 },
	{ 156,101,0 },
	{ 156,101,49 },
	{ 156,101,98 },
	{ 156,101,156 },
	{ 156,101,205 },
	{ 156,101,255 },
	{ 156,153,0 },
	{ 156,153,49 },
	{ 156,153,98 },
	{ 156,153,156 },
	{ 156,153,205 },
	{ 156,153,255 },
	{ 156,206,0 },
	{ 156,206,49 },
	{ 156,206,98 },
	{ 156,206,156 },
	{ 156,206,205 },
	{ 156,206,255 },
	{ 156,255,0 },
	{ 156,255,49 },
	{ 156,255,98 },
	{ 156,255,156 },
	{ 156,255,205 },
	{ 156,255,255 },
	{ 205,0,0 },
	{ 205,0,49 },
	{ 205,0,98 },
	{ 205,0,156 },
	{ 205,0,205 },
	{ 205,0,255 },
	{ 205,48,0 },
	{ 205,48,49 },
	{ 205,48,98 },
	{ 205,48,156 },
	{ 205,48,205 },
	{ 205,48,255 },
	{ 205,101,0 },
	{ 205,101,49 },
	{ 205,101,98 },
	{ 205,101,156 },
	{ 205,101,205 },
	{ 205,101,255 },
	{ 205,153,0 },
	{ 205,153,49 },
	{ 205,153,98 },
	{ 205,153,156 },
	{ 205,153,205 },
	{ 205,153,255 },
	{ 205,206,0 },
	{ 205,206,49 },
	{ 205,206,98 },
	{ 205,206,156 },
	{ 205,206,205 },
	{ 205,206,255 },
	{ 205,255,0 },
	{ 205,255,49 },
	{ 205,255,98 },
	{ 205,255,156 },
	{ 205,255,205 },
	{ 205,255,255 },
	{ 255,0,0 },
	{ 255,0,49 },
	{ 255,0,98 },
	{ 255,0,156 },
	{ 255,0,205 },
	{ 255,0,255 },
	{ 255,48,0 },
	{ 255,48,49 },
	{ 255,48,98 },
	{ 255,48,156 },
	{ 255,48,205 },
	{ 255,48,255 },
	{ 255,101,0 },
	{ 255,101,49 },
	{ 255,101,98 },
	{ 255,101,156 },
	{ 255,101,205 },
	{ 255,101,255 },
	{ 255,153,0 },
	{ 255,153,49 },
	{ 255,153,98 },
	{ 255,153,156 },
	{ 255,153,205 },
	{ 255,153,255 },
	{ 255,206,0 },
	{ 255,206,49 },
	{ 255,206,98 },
	{ 255,206,156 },
	{ 255,206,205 },
	{ 255,206,255 },
	{ 255,255,0 },
	{ 255,255,49 },
	{ 255,255,98 },
	{ 255,255,156 },
	{ 255,255,205 },
	{ 255,255,255 }
};
#endif /* vMacScreenDepth */

VESA_INFO VESA_info;
MODE_INFO mode_info;
int VESA_previousBank;

static int VESA_GetInfo(void) {
	__dpmi_regs r;
	long dosbuf;
	int c;

	dosbuf = __tb & 0xFFFFF;

	for (c = 0; c < sizeof(VESA_INFO); c++)
		_farpokeb(_dos_ds, dosbuf+c, 0);
	
	dosmemput("VBE2", 4, dosbuf);

	r.x.ax = 0x4F00;
	r.x.di = dosbuf & 0xF;
	r.x.es = (dosbuf >> 4) & 0xFFFF;
	__dpmi_int(0x10, &r);

	if (r.h.ah)
		return -1;

	dosmemget(dosbuf, sizeof(VESA_INFO), &VESA_info);

	if (strncmp(VESA_info.VESASignature, "VESA", 4) != 0)
		return -1;

	return 0;
}

static int VESA_GetModeInfo(int mode) {
	__dpmi_regs r;
	long dosbuf;
	int c;

	dosbuf = __tb & 0xFFFFF;

	for (c=0; c<sizeof(MODE_INFO); c++)
		_farpokeb(_dos_ds, dosbuf+c, 0);

	r.x.ax = 0x4F01;
	r.x.di = dosbuf & 0xF;
	r.x.es = (dosbuf >> 4) & 0xFFFF;
	r.x.cx = mode;
	__dpmi_int(0x10, &r);

	if (r.h.ah)
		return -1;

	dosmemget(dosbuf, sizeof(MODE_INFO), &mode_info);

	return 0;
}

static int VESA_SetMode(int mode) {
	__dpmi_regs r;

	r.x.ax = 0x4F02;
	r.x.bx = mode;
	__dpmi_int(0x10, &r);
	if (r.h.ah)
		return -1;

	return 0;
}

LOCALPROC VESA_SetBank(int bank_number) {
	if (bank_number != VESA_previousBank) {
		__dpmi_regs r;
		
		r.x.ax = 0x4F05;
		r.x.bx = 0;
		r.x.dx = bank_number;
		__dpmi_int(0x10, &r);
		VESA_previousBank = bank_number;
	}
}

#if vMacScreenDepth > 0
LOCALPROC VESA_DrawPixel(char *memory_buffer, int x, int y, int color) {
	int address = y*VESAWidth+x;
	
	memory_buffer[address] = color;
}
#endif /* vMacScreenDepth */

LOCALPROC VESA_CopyFBToScreen(unsigned char *memory_buffer, int screen_size) {
	int bank_size = mode_info.WinSize*1024;
	int bank_granularity = mode_info.WinGranularity*1024;
	int bank_number = 0;
	int todo = screen_size;
	int copy_size;

	while (todo > 0) {
		VESA_SetBank(bank_number);

		if (todo > bank_size)
			copy_size = bank_size;
		else
			copy_size = todo;

		dosmemput(memory_buffer, copy_size, 0xA0000);

		todo -= copy_size;
		memory_buffer += copy_size;
		bank_number += bank_size/bank_granularity;
	}
}

LOCALPROC Video_SetColor(int color, int red, int green, int blue) {
	outportb(0x3C8, color);
	outportb(0x3C9, red);
	outportb(0x3C9, green);
	outportb(0x3C9, blue);
}

LOCALPROC HaveChangedScreenBuff(ui4r top, ui4r left, ui4r bottom, ui4r right) {
	ui3r *drawBuffer = GetCurDrawBuff();
	ui3r *fBuffer = FrameBuffer;
	ui5r currentPixel;
	
	for (unsigned int i = 0; i < (bottom - top) + renderOffset[1]; i++) {
		for (unsigned int j = 0; j < VESAWidth / 8; j++) {
			if (j < renderOffset[0] / 8 || i < renderOffset[1]) {
				currentPixel = 0xff;
			} else if (j < (vMacScreenWidth + renderOffset[0]) / 8) {
				currentPixel = *drawBuffer++;
			} else {
				currentPixel = 0xff;
			}
			for (int k = 8; --k >= 0;) {
				*fBuffer++ = ((currentPixel >> k) & 1) - 1;
			}
		}
	}
	
	VESA_CopyFBToScreen(FrameBuffer, sizeof(FrameBuffer));
}

LOCALPROC MyDrawChangesAndClear(void) {
	if (ScreenChangedBottom > ScreenChangedTop) {
		HaveChangedScreenBuff(ScreenChangedTop, ScreenChangedLeft, ScreenChangedBottom, ScreenChangedRight);
		ScreenClearChanges();
	}
}

/* --- mouse --- */
__dpmi_regs r;

LOCALPROC MousePositionNotify(int NewMousePosh, int NewMousePosv) {
	NewMousePosh -= renderOffset[0];
	NewMousePosv -= renderOffset[1];

	MyMousePositionSet(NewMousePosh, NewMousePosv);
}

LOCALPROC CheckMouseState(void) {
	r.x.ax = 3;	
	__dpmi_int(0x33, &r);
	MyMouseButtonSet(r.x.bx);
	int mouseX = r.x.cx;
	int mouseY = r.x.dx;
	
	MousePositionNotify(mouseX, mouseY);
}

/* --- keyboard input --- */

LOCALVAR volatile int LastKeyboardKey = MKC_None;
LOCALVAR volatile int KeyboardKey = MKC_None;
LOCALVAR unsigned int CapsLock = falseblnr;
LOCALVAR unsigned char KC2MKC[256] = {
	MKC_None, MKC_formac_Escape, MKC_1, MKC_2, MKC_3, MKC_4, MKC_5, MKC_6, MKC_7, MKC_8, MKC_9, MKC_0, MKC_Minus, MKC_Equal, MKC_BackSpace, MKC_Tab, MKC_Q, MKC_W, MKC_E, MKC_R, MKC_T, MKC_Y, MKC_U, MKC_I, MKC_O, MKC_P, MKC_LeftBracket, MKC_RightBracket, MKC_formac_Enter, MKC_formac_Control, MKC_A, MKC_S, MKC_D, MKC_F, MKC_G, MKC_H, MKC_J, MKC_K, MKC_L, MKC_SemiColon, MKC_SingleQuote, MKC_formac_Grave, MKC_formac_Shift, MKC_formac_Slash, MKC_Z, MKC_X, MKC_C, MKC_V, MKC_B, MKC_N, MKC_M, MKC_Comma, MKC_Period, MKC_formac_Slash, MKC_formac_Shift, MKC_KPMultiply, MKC_formac_Option, MKC_Space, MKC_formac_CapsLock, MKC_formac_F1, MKC_formac_F2, MKC_formac_F3, MKC_formac_F4, MKC_formac_F5, MKC_F6, MKC_F7, MKC_F8, MKC_F9, MKC_F10, MKC_Clear, MKC_ScrollLock, MKC_formac_Home, MKC_Up, MKC_formac_PageUp, MKC_KPSubtract, MKC_Left, MKC_KP5, MKC_Right, MKC_KPAdd, MKC_formac_End, MKC_Down, MKC_formac_PageDown, MKC_formac_Help, MKC_formac_ForwardDel, MKC_None, MKC_None, MKC_formac_BackSlash, MKC_F11, MKC_F12, MKC_KPEqual, MKC_None, MKC_formac_Command, MKC_formac_Command, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None,
	MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None, MKC_None
};
unsigned char KeyDown[sizeof(KC2MKC)];

LOCALPROC CheckTheCapsLock(void) {
	Keyboard_UpdateKeyMap2(0x39, CapsLock); /* 0x39 = Caps Lock (Mac Keycode) */
}

LOCALPROC DoKeyCode(unsigned char keycode, blnr down) { 
	/* Stub */
}

LOCALPROC CheckKeyboardState(void) {
	/* Stub */
}

/* --- time, date, location --- */

#define dbglog_TimeStuff (0 && dbglog_HAVE)

LOCALVAR ui5b TrueEmulatedTime = 0;

#include "DATE2SEC.h"

#define TicksPerSecond 1000000

LOCALVAR blnr HaveTimeDelta = falseblnr;
LOCALVAR ui5b TimeDelta;

LOCALVAR ui5b NewMacDateInSeconds;

LOCALVAR ui5b LastTimeSec;
LOCALVAR ui5b LastTimeUsec;

LOCALPROC GetCurrentTicks(void) {
	struct timeval t;

	gettimeofday(&t, NULL);
	if (! HaveTimeDelta) {
		time_t Current_Time;
		struct tm *s;

		(void) time(&Current_Time);
		s = localtime(&Current_Time);
		TimeDelta = Date2MacSeconds(s->tm_sec, s->tm_min, s->tm_hour,
			s->tm_mday, 1 + s->tm_mon, 1900 + s->tm_year) - t.tv_sec;
#if 0 && AutoTimeZone /* how portable is this ? */
		CurMacDelta = ((ui5b)(s->tm_gmtoff) & 0x00FFFFFF)
			| ((s->tm_isdst ? 0x80 : 0) << 24);
#endif
		HaveTimeDelta = trueblnr;
	}

	NewMacDateInSeconds = t.tv_sec + TimeDelta;
	LastTimeSec = (ui5b)t.tv_sec;
	LastTimeUsec = (ui5b)t.tv_usec;
}

#define MyInvTimeStep 16626 /* TicksPerSecond / 60.14742 */

LOCALVAR ui5b NextTimeSec;
LOCALVAR ui5b NextTimeUsec;

LOCALPROC IncrNextTime(void) {
	NextTimeUsec += MyInvTimeStep;
	if (NextTimeUsec >= TicksPerSecond) {
		NextTimeUsec -= TicksPerSecond;
		NextTimeSec += 1;
	}
}

LOCALPROC InitNextTime(void) {
	NextTimeSec = LastTimeSec;
	NextTimeUsec = LastTimeUsec;
	IncrNextTime();
}

LOCALPROC StartUpTimeAdjust(void) {
	GetCurrentTicks();
	InitNextTime();
}

LOCALFUNC si5b GetTimeDiff(void) {
	return ((si5b)(LastTimeSec - NextTimeSec)) * TicksPerSecond
		+ ((si5b)(LastTimeUsec - NextTimeUsec));
}

LOCALPROC UpdateTrueEmulatedTime(void) {
	si5b TimeDiff;

	GetCurrentTicks();

	TimeDiff = GetTimeDiff();
	if (TimeDiff >= 0) {
		if (TimeDiff > 16 * MyInvTimeStep) {
			/* emulation interrupted, forget it */
			++TrueEmulatedTime;
			InitNextTime();

#if dbglog_TimeStuff
			dbglog_writelnNum("emulation interrupted",
				TrueEmulatedTime);
#endif
		} else {
			do {
				++TrueEmulatedTime;
				IncrNextTime();
				TimeDiff -= TicksPerSecond;
			} while (TimeDiff >= 0);
		}
	} else if (TimeDiff < - 16 * MyInvTimeStep) {
		/* clock goofed if ever get here, reset */
#if dbglog_TimeStuff
		dbglog_writeln("clock set back");
#endif

		InitNextTime();
	}
}

LOCALFUNC blnr CheckDateTime(void) {
	if (CurMacDateInSeconds != NewMacDateInSeconds) {
		CurMacDateInSeconds = NewMacDateInSeconds;
		return trueblnr;
	} else {
		return falseblnr;
	}
}

/* --- sound --- */

/*
	"I'm not going to be adding sound support because I think it would be too hard for me to add like Sound Blaster support or something, if someone reading this thinks they can, go ahead if you want."
		- Steven (StevenSYS)
*/

/* --- basic dialogs --- */

LOCALPROC CheckSavedMacMsg(void) {
	if (nullpr != SavedBriefMsg) {
		char briefMsg0[ClStrMaxLength + 1];
		char longMsg0[ClStrMaxLength + 1];

		NativeStrFromCStr(briefMsg0, SavedBriefMsg);
		NativeStrFromCStr(longMsg0, SavedLongMsg);

		fprintf(stderr, "%s\n", briefMsg0);
		fprintf(stderr, "%s\n", longMsg0);

		SavedBriefMsg = nullpr;
	}
}

/* This is only here because it won't compile without it */
LOCALPROC ToggleWantFullScreen(void) {
	return;
}

/* --- SavedTasks --- */

LOCALPROC LeaveSpeedStopped(void) {
/*#if MySoundEnabled
	MySound_Start();
#endif*/

	StartUpTimeAdjust();
}

LOCALPROC EnterSpeedStopped(void) {
/*#if MySoundEnabled
	MySound_Stop();
#endif*/
}

LOCALPROC CheckForSavedTasks(void) {
	if (MyEvtQNeedRecover) {
		MyEvtQNeedRecover = falseblnr;

		/* attempt cleanup, MyEvtQNeedRecover may get set again */
		MyEvtQTryRecoverFromFull();
	}

	if (RequestMacOff) {
		RequestMacOff = falseblnr;
		if (AnyDiskInserted()) {
			MacMsgOverride(kStrQuitWarningTitle,
				kStrQuitWarningMessage);
		} else {
			ForceMacOff = trueblnr;
		}
	}

	if (ForceMacOff) {
		return;
	}
	
	if (CurSpeedStopped != SpeedStopped) {
		CurSpeedStopped = ! CurSpeedStopped;
		if (CurSpeedStopped) {
			EnterSpeedStopped();
		} else {
			LeaveSpeedStopped();
		}
	}

#if IncludeSonyNew
	if (vSonyNewDiskWanted) {
#if IncludeSonyNameNew
		if (vSonyNewDiskName != NotAPbuf) {
			ui3p NewDiskNameDat;
			if (MacRomanTextToNativePtr(vSonyNewDiskName, trueblnr,
				&NewDiskNameDat))
			{
				MakeNewDisk(vSonyNewDiskSize, (char *)NewDiskNameDat);
				free(NewDiskNameDat);
			}
			PbufDispose(vSonyNewDiskName);
			vSonyNewDiskName = NotAPbuf;
		} else
#endif
		{
			MakeNewDiskAtDefault(vSonyNewDiskSize);
		}
		vSonyNewDiskWanted = falseblnr;
			/* must be done after may have gotten disk */
	}
#endif

	if ((nullpr != SavedBriefMsg) & ! MacMsgDisplayed) {
		MacMsgDisplayOn();
	}

	if (NeedWholeScreenDraw) {
		NeedWholeScreenDraw = falseblnr;
		ScreenChangedAll();
	}

#if NeedRequestIthDisk
	if (0 != RequestIthDisk) {
		Sony_InsertIth(RequestIthDisk);
		RequestIthDisk = 0;
	}
#endif
}

/* --- main program flow --- */

GLOBALOSGLUPROC DoneWithDrawingForTick(void) {
#if EnableFSMouseMotion
	if (HaveMouseMotion) {
		AutoScrollScreen();
	}
#endif
	MyDrawChangesAndClear();
}

GLOBALOSGLUFUNC blnr ExtraTimeNotOver(void) {
	UpdateTrueEmulatedTime();
	return TrueEmulatedTime == OnTrueTime;
}

LOCALPROC CheckForSystemEvents(void) {
	CheckKeyboardState();
}

GLOBALOSGLUPROC WaitForNextTick(void) {
label_retry:
	// CheckForSystemEvents();
	CheckForSavedTasks();
	if (ForceMacOff) {
		return;
	}

	if (CurSpeedStopped) {
		DoneWithDrawingForTick();
		goto label_retry;
	}

	if (ExtraTimeNotOver()) {
		/*
			FIXME:

			Implement this?

			struct timespec rqt;
			struct timespec rmt;

			rqt.tv_sec = 0;
			rqt.tv_nsec = (- TimeDiff) * 1000;

			(void) nanosleep(&rqt, &rmt);
		*/
	}

	if (CheckDateTime()) {
/*#if MySoundEnabled
		MySound_SecondNotify();
#endif*/
#if EnableDemoMsg
		DemoModeSecondNotify();
#endif
	}

	CheckMouseState();
	
	CheckKeyboardState();
	
	OnTrueTime = TrueEmulatedTime;

#if dbglog_TimeStuff
	dbglog_writelnNum("WaitForNextTick, OnTrueTime", OnTrueTime);
#endif
}

/* --- platform independent code can be thought of as going here --- */

#include "PROGMAIN.h"

LOCALPROC ReserveAllocAll(void) {
#if dbglog_HAVE
	dbglog_ReserveAlloc();
#endif
	ReserveAllocOneBlock(&ROM, kROM_Size, 5, falseblnr);

	ReserveAllocOneBlock(&screencomparebuff,
		vMacScreenNumBytes, 5, trueblnr);
#if UseControlKeys
	ReserveAllocOneBlock(&CntrlDisplayBuff,
		vMacScreenNumBytes, 5, falseblnr);
#endif

/*#if MySoundEnabled
	ReserveAllocOneBlock((ui3p *)&TheSoundBuffer,
		dbhBufferSize, 5, falseblnr);
#endif*/

	EmulationReserveAlloc();
}

LOCALFUNC blnr AllocMyMemory(void) {
	uimr n;
	blnr IsOk = falseblnr;

	ReserveAllocOffset = 0;
	ReserveAllocBigBlock = nullpr;
	ReserveAllocAll();
	n = ReserveAllocOffset;
	ReserveAllocBigBlock = (ui3p)calloc(1, n);
	if (NULL == ReserveAllocBigBlock) {
		MacMsg(kStrOutOfMemTitle, kStrOutOfMemMessage, trueblnr);
	} else {
		ReserveAllocOffset = 0;
		ReserveAllocAll();
		if (n != ReserveAllocOffset) {
			/* oops, program error */
		} else {
			IsOk = trueblnr;
		}
	}

	return IsOk;
}

LOCALPROC UnallocMyMemory(void) {
	if (nullpr != ReserveAllocBigBlock) {
		free((char *)ReserveAllocBigBlock);
	}
}

LOCALFUNC blnr InitOSGLU(void) {
	memset(KeyDown, 0, sizeof(KeyDown));

	if (ScanCommandLine())
	
	/* Set the mouse sensitivity */
	r.x.ax = 26; 
	r.x.bx = MouseSenX;
	r.x.cx = MouseSenY;
	__dpmi_int(0x33, &r);

	/* Set the min and max horizontal cursor position */
	r.x.ax = 7;
	r.x.cx = 0;
	if (mouseEdgeStick) /* This is a way of doing this */
		r.x.dx = vMacScreenWidth + renderOffset[0];
	else
		r.x.dx = VESAWidth;
	__dpmi_int(0x33, &r);
	
	/* Set the min and max vertical cursor position */
	r.x.ax = 8;
	r.x.cx = 0;
	if (mouseEdgeStick)
		r.x.dx = vMacScreenHeight + renderOffset[1];
	else
		r.x.dx = VESAHeight;
	__dpmi_int(0x33, &r);
	
	memset(FrameBuffer, 0, sizeof(FrameBuffer));
	
	if (VESA_GetInfo()) {
		printf("This GPU doesn't support VBE 2.0");
		return falseblnr;
	}

	if (VESA_SetMode(VESAMode)) {
		printf("Failed to set VESA mode");
		return falseblnr;
	}

	VESA_GetModeInfo(VESAMode);
	#if vMacScreenDepth > 0
	for (int i = 0; i < 256; i++) {
		Video_SetColor(i, video_colorPalette[i][0] / 4, video_colorPalette[i][1] / 4, video_colorPalette[i][2] / 4);
	}
	#endif /* vMacScreenDepth */
	
	#if vMacScreenDepth == 0
	Video_SetColor(255, 63, 63, 63);
	#endif /* vMacScreenDepth */
	
	if (LoadInitialImages())
	if (AllocMyMemory())
	if (LoadMacRom())
	if (WaitForRom()) {
		return trueblnr;
	}
	return falseblnr;
}

LOCALPROC UnInitOSGLU(void) {
	if (MacMsgDisplayed) {
		MacMsgDisplayOff();
	}
	
	__dpmi_regs r;
	r.x.ax = 03;
	__dpmi_int(0x10, &r);
	
	UnInitDrives();
	
#if dbglog_HAVE
	dbglog_close();
#endif

	UnallocMyMemory();

	CheckSavedMacMsg();
}

int main(int argc, char *argv[]) {
	my_argc = argc;
	my_argv = argv;

	if (InitOSGLU()) {
		ProgramMain();
	}
	UnInitOSGLU();
	
	return 0;
}

#endif /* WantOSGLUDOS */