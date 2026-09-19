/* DaynaPORT SCSI/Link target, included by SCSIDNPT.h.
   SPDX-License-Identifier: GPL-2.0-only

   Protocol references: BlueSCSI-v2 firmware/network.c and the
   DaynaPort SCSI Link command documentation by Roger Burrows / PiSCSI.
   This implementation sends one receive record per READ, including in
   blind mode. Its last-record flag is therefore always zero. */
#include "DAYNAAPI.h"
#include <string.h>

#define DN_FRAME_MAX 1514
#define DN_BUFFER_SIZE (DN_FRAME_MAX + 10)

enum { DN_STATUS, DN_INPUT, DN_OUTPUT };
static uint8_t dn_buffer[DN_BUFFER_SIZE];
static uint8_t dn_cdb[16];
static size_t dn_length;
static int dn_direction;
static uint8_t dn_status, dn_sense;
static int dn_enabled;
static int dn_write_header;

static void DNReset(void)
{
	dn_enabled = 0;
	dn_status = dn_sense = 0;
	dn_length = 0;
	dn_direction = DN_STATUS;
	dn_write_header = 0;
}

static void DNError(uint8_t sense)
{
	dn_status = 2;
	dn_sense = sense;
	dn_direction = DN_STATUS;
	dn_length = 0;
}

static void DNTransfer(int direction, size_t length)
{
	dn_length = length;
	dn_direction = length ? direction : DN_STATUS;
}

static uint32_t DNCRC(const uint8_t *p, size_t length)
{
	uint32_t crc = UINT32_C(0xffffffff);
	unsigned bit;
	while (length--) {
		crc ^= *p++;
		for (bit = 0; bit < 8; ++bit) {
			crc = (crc >> 1) ^ ((crc & 1) ? UINT32_C(0xedb88320) : 0);
		}
	}
	return ~crc;
}

static void DNCommand(void)
{
	size_t n, size = ((size_t)dn_cdb[3] << 8) | dn_cdb[4];
	uint32_t crc;
	uint8_t mac[6];
	unsigned i;
	dn_status = 0;
	dn_direction = DN_STATUS;
	dn_length = 0;
	dn_write_header = 0;
	memset(dn_buffer, 0, sizeof(dn_buffer));
	if ((dn_cdb[1] & 0xe0) && dn_cdb[0] != 0x12) {
		DNError(5);
		return;
	}
	switch (dn_cdb[0]) {
	case 0x00: /* TEST UNIT READY */
		if (!DaynaHostGetMAC(mac)) DNError(2);
		break;
	case 0x03: /* Dayna's zero allocation REQUEST SENSE returns nine bytes. */
		dn_buffer[0] = 0x70;
		dn_buffer[2] = dn_sense;
		dn_buffer[7] = 1;
		DNTransfer(DN_INPUT, dn_cdb[4] ? (dn_cdb[4] < 9 ? dn_cdb[4] : 9) : 9);
		dn_sense = 0;
		break;
	case 0x12:
		if (dn_cdb[1] & 1) { DNError(5); break; }
		dn_buffer[0] = (dn_cdb[1] & 0xe0) ? 0x7f : 3;
		dn_buffer[2] = 1;
		dn_buffer[4] = 31;
		memcpy(dn_buffer + 8, "Dayna   SCSI/Link       2.0f", 28);
		DNTransfer(DN_INPUT, dn_cdb[4] < 37 ? dn_cdb[4] : 37);
		break;
	case 0x08:
		/* The boot-time size-one probe must fail without a data phase. */
		if (size < 6) { DNError(5); break; }
		n = dn_enabled ? DaynaHostReceive(dn_buffer + 6, DN_FRAME_MAX) : 0;
		if (n > DN_FRAME_MAX || (n && n < 14)) n = 0;
		if (n) {
			/* Ethernet minimum excludes FCS; host backends may omit padding. */
			if (n < 60) n = 60;
			if (n + 10 > size) { DNError(5); break; }
			crc = DNCRC(dn_buffer + 6, n);
			for (i = 0; i < 4; ++i) dn_buffer[6 + n + i] = (uint8_t)(crc >> (8 * i));
			n += 4;
			dn_buffer[0] = (uint8_t)(n >> 8);
			dn_buffer[1] = (uint8_t)n;
		}
		DNTransfer(DN_INPUT, n + 6);
		break;
	case 0x09:
		if (!DaynaHostGetMAC(dn_buffer)) { DNError(2); break; }
		/* Hardware returns all 18 bytes even for the diagnostics size-six CDB. */
		DNTransfer(DN_INPUT, 18);
		break;
	case 0x0a:
		if (!dn_enabled) { DNError(2); break; }
		if (dn_cdb[5] == 0) {
			if (size < 14 || size > DN_FRAME_MAX) { DNError(5); break; }
			DNTransfer(DN_OUTPUT, size);
		} else if (dn_cdb[5] == 0x80) {
			/* Four-byte record headers; a zero-length record terminates. */
			dn_write_header = 1;
			DNTransfer(DN_OUTPUT, 4);
		} else DNError(5);
		break;
	case 0x0c: /* Receive mode. MAC overrides cannot be honored by vmnet. */
		if (dn_cdb[5] == 0x40) DNError(5);
		break;
	case 0x0d: /* Consume multicast filter data; backend supplies multicast. */
		if (size > DN_BUFFER_SIZE || size % 6) { DNError(5); break; }
		DNTransfer(DN_OUTPUT, size);
		break;
	case 0x0e:
		dn_enabled = (dn_cdb[5] & 0x80) != 0;
		/* Discard old packets on either transition, with bounded work. */
		for (i = 0; i < 64 && DaynaHostReceive(dn_buffer, DN_FRAME_MAX); ++i) {}
		break;
	case 0x1a: /* Matches BlueSCSI's no-data MODE SENSE. */
	case 0x80:
		break;
	default:
		DNError(5);
		break;
	}
}

/* Called after each complete DATA OUT record, not just at command end. */
static void DNOutputDone(void)
{
	size_t n;
	if (dn_cdb[0] == 0x0a) {
		if (dn_write_header) {
			n = ((size_t)dn_buffer[0] << 8) | dn_buffer[1];
			if (!n) { DNTransfer(DN_STATUS, 0); return; }
			if (n < 14 || n > DN_FRAME_MAX) { DNError(5); return; }
			dn_write_header = 0;
			DNTransfer(DN_OUTPUT, n);
			return;
		}
		if (!DaynaHostSend(dn_buffer, dn_length)) { DNError(4); return; }
		if (dn_cdb[5] == 0x80) {
			dn_write_header = 1;
			DNTransfer(DN_OUTPUT, 4);
			return;
		}
	}
	DNTransfer(DN_STATUS, 0);
}
