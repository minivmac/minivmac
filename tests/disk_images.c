/* Exercise the production image detector with synthetic disk headers.
   No ROM, guest image or host GUI is required.
   SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/SONYEMDV.c"

#if ! NonDiskProtect || ! Sony_SupportDC42 || Sony_VerifyChecksums
#error "Tests require disk format detection and runtime checksum verification disabled"
#endif

static ui3b header[checkheadersize];
static ui5r image_size;
static unsigned ejections, warnings;
GLOBALVAR ui5b vSonyInsertedMask;
GLOBALVAR ui5b vSonyWritableMask;
GLOBALVAR blnr vSonyRawMode;

GLOBALOSGLUFUNC tMacErr vSonyGetSize(tDrive drive, ui5r *size)
{
	assert(drive == 0);
	*size = image_size;
	return mnvm_noErr;
}
GLOBALOSGLUFUNC tMacErr vSonyTransfer(blnr write, ui3p buffer,
	tDrive drive, ui5r start, ui5r count, ui5r *actual)
{
	assert(!write && drive == 0 && start == 0 && count == sizeof(header));
	memcpy(buffer, header, count);
	if (actual) *actual = count;
	return mnvm_noErr;
}
GLOBALOSGLUFUNC tMacErr vSonyEject(tDrive drive)
{
	assert(drive == 0);
	vSonyInsertedMask = 0;
	++ejections;
	return mnvm_noErr;
}
GLOBALOSGLUPROC WarnMsgUnsupportedDisk(void) { ++warnings; }
GLOBALOSGLUPROC DiskRevokeWritable(tDrive drive)
{
	vSonyWritableMask &= ~((ui5b)1 << drive);
}

static void reset_image(ui5r size)
{
	memset(header, 0, sizeof(header));
	image_size = size;
	vSonyInsertedMask = vSonyWritableMask = 1;
	vSonyMountedMask = 0;
	ejections = warnings = 0;
}
static void partition(unsigned entry, ui5r start, ui5r count)
{
	ui3p map = header + 512 * entry;
	do_put_mem_word(map, 0x504d);
	do_put_mem_long(map + 8, start);
	do_put_mem_long(map + 12, count);
	memcpy(map + 48, "Apple_HFS", 10);
}
static void check_mount(blnr expected, ui5r offset, ui5r size)
{
	tDrive drive = 99;
	tMacErr result = vSonyNextPendingInsert(&drive);
	if (expected) {
		assert(result == mnvm_noErr && drive == 0);
		assert(vSonyIsMounted(0) && !ejections && !warnings);
		assert(ImageDataOffset[0] == offset && ImageDataSize[0] == size);
	} else {
		assert(result != mnvm_noErr && drive == 99);
		assert(!vSonyIsMounted(0) && ejections == 1 && warnings == 1);
	}
}
static void dc42(ui5r data, ui5r tags)
{
	do_put_mem_word(header + kDC42offset_private, 0x0100);
	do_put_mem_long(header + kDC42offset_dataSize, data);
	do_put_mem_long(header + kDC42offset_tagSize, tags);
}
static void valid_images(void)
{
	/* Raw HFS and MFS retain their original mount paths. */
	reset_image(128 * 512);
	do_put_mem_word(header + 0x400, 0x4244);
	do_put_mem_word(header + 0x40e, 3);
	check_mount(trueblnr, 0, image_size);
	reset_image(128 * 512);
	do_put_mem_word(header + 0x400, 0xd2d7);
	check_mount(trueblnr, 0, image_size);

	/* Partition ending exactly at EOF, including the last scanned entry. */
	reset_image(128 * 512);
	partition(1, 64, 64);
	check_mount(trueblnr, 64 * 512, 64 * 512);
	reset_image(128 * 512);
	partition(checkheaderblocks - 1, 64, 64);
	check_mount(trueblnr, 64 * 512, 64 * 512);

	/* Ordinary Disk Copy images with and without tags. */
	reset_image(84 + 800 * 512);
	dc42(800 * 512, 0);
	check_mount(trueblnr, 84, 800 * 512);
	reset_image(84 + 800 * 512 + 800 * 12);
	dc42(800 * 512, 800 * 12);
	check_mount(trueblnr, 84, 800 * 512);
}
static void partition_ranges(void)
{
	reset_image(512); /* Too short for the image header probe. */
	check_mount(falseblnr, 0, 0);
	reset_image(128 * 512);
	partition(1, 128, 1); /* Starts at EOF, but requests a block. */
	check_mount(falseblnr, 0, 0);
	reset_image(128 * 512);
	partition(1, 129, 1); /* Starts beyond EOF. */
	check_mount(falseblnr, 0, 0);
	reset_image(128 * 512);
	partition(1, 64, 65); /* Ends beyond EOF. */
	check_mount(falseblnr, 0, 0);
	reset_image(128 * 512);
	partition(1, 64, 0); /* Empty partition. */
	check_mount(falseblnr, 0, 0);
	reset_image(128 * 512);
	partition(1, 0x00800040, 64); /* Byte offset wraps to a valid-looking 32 KB. */
	check_mount(falseblnr, 0, 0);
	reset_image(128 * 512);
	partition(1, 64, 0x00800040); /* Byte length wraps similarly. */
	check_mount(falseblnr, 0, 0);
	reset_image(128 * 512);
	partition(1, 0xffffffff, 0xffffffff);
	check_mount(falseblnr, 0, 0);

	/* Invalid entries must not hide a later valid HFS partition. */
	reset_image(128 * 512);
	partition(1, 129, 1);
	partition(2, 64, 64);
	check_mount(trueblnr, 64 * 512, 64 * 512);
}
static void large_partitions(void)
{
	/* Virtual file sizes avoid allocating multi-gigabyte test images. */
	reset_image(0x80008000U);
	partition(1, 0x00400000, 64);
	check_mount(trueblnr, 0x80000000U, 64 * 512);
	reset_image(0x80008000U);
	partition(1, 64, 0x00400000);
	check_mount(trueblnr, 64 * 512, 0x80000000U);
	reset_image(0xffffffffU);
	partition(1, 64, 0x007fffbf);
	check_mount(trueblnr, 64 * 512, 0xffff7e00U);
}
static void dc42_ranges(void)
{
	ui5r data = 0xfe000000U;
	ui5r tags = (data >> 9) * 12;
	reset_image(84 + 800 * 512 - 1);
	dc42(800 * 512, 0); /* Truncated data. */
	check_mount(falseblnr, 0, 0);
	reset_image(84 + 800 * 512 + 800 * 12 - 1);
	dc42(800 * 512, 800 * 12); /* Truncated tags. */
	check_mount(falseblnr, 0, 0);
	reset_image(84 + data + tags); /* Total wraps, appearing to fit. */
	dc42(data, tags);
	check_mount(falseblnr, 0, 0);
}
int main(int argc, char **argv)
{
	const char *group = argc > 1 ? argv[1] : "all";
	if (!strcmp(group, "all") || !strcmp(group, "valid")) valid_images();
	if (!strcmp(group, "all") || !strcmp(group, "partition-range")) partition_ranges();
	if (!strcmp(group, "all") || !strcmp(group, "large-partition")) large_partitions();
	if (!strcmp(group, "all") || !strcmp(group, "dc42-range")) dc42_ranges();
	puts("Disk image bounds tests passed");
	return 0;
}
