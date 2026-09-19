/* Verify that a valid empty parameter buffer does not depend on the
   implementation-defined result of calloc(0, ...). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/OSGCOMUI.h"
#include "../src/OSGCOMUD.h"
#include "../src/COMOSGLU.h"
#include "../src/PBUFSTDC.h"

GLOBALOSGLUPROC MyMoveBytes(anyp srcPtr, anyp destPtr, si5b byteCount)
{
	(void)memmove(destPtr, srcPtr, (size_t)byteCount);
}

int main(void)
{
	tPbuf pbuf;
	ui5r size;

	assert(mnvm_noErr == PbufNew(0, &pbuf));
	assert(mnvm_noErr == PbufGetSize(pbuf, &size));
	assert(0 == size);
	PbufDispose(pbuf);

	puts("Zero-length parameter buffer test passed");
	return 0;
}
