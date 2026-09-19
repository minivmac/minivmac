/* NCR5380 initiator interface for one DaynaPORT target at SCSI ID 4.
   Included only in experimental DaynaPORT builds.
   SPDX-License-Identifier: GPL-2.0-only */
#include "DAYNAPRT.h"

enum { SD_OUT = 0, SD_IN = 1, SD_COMMAND = 2, SD_STATUS = 3,
	SD_MSGOUT = 6, SD_MSGIN = 7, SD_FREE = 8, SD_SELECTED = 9 };
static uint8_t sd_odr, sd_icr, sd_mode, sd_tcr;
static unsigned sd_phase, sd_cmdlength;
static size_t sd_pos;
static uint8_t sd_latch;
static int sd_ack, sd_dma, sd_irq, sd_enddma;

static void SDPhase(unsigned phase)
{
	sd_phase = phase;
	sd_pos = 0;
	if (sd_dma && phase < SD_FREE && phase != (sd_tcr & 7)) {
		sd_enddma = sd_irq = 1;
		sd_dma = 0;
	}
}

GLOBALPROC SCSI_Reset(void)
{
	sd_odr = sd_icr = sd_mode = sd_tcr = 0;
	sd_ack = sd_dma = sd_irq = sd_enddma = 0;
	sd_cmdlength = 0;
	SDPhase(SD_FREE);
	DNReset();
}

static void SDTargetPhase(void)
{
	SDPhase(dn_direction == DN_INPUT ? SD_IN :
		dn_direction == DN_OUTPUT ? SD_OUT : SD_STATUS);
}

static uint8_t SDData(void)
{
	switch (sd_phase) {
	case SD_IN: return dn_buffer[sd_pos];
	case SD_STATUS: return dn_status;
	case SD_MSGIN: return 0; /* COMMAND COMPLETE */
	default: return sd_odr;
	}
}

static void SDByteDone(uint8_t data)
{
	switch (sd_phase) {
	case SD_COMMAND:
		if (!sd_pos) {
			unsigned group = data >> 5;
			sd_cmdlength = (group == 0 || data == 0x40 || data == 0x80) ? 6 :
				(group == 4 ? 16 : (group == 5 ? 12 : 10));
			memset(dn_cdb, 0, sizeof(dn_cdb));
		}
		dn_cdb[sd_pos++] = data;
		if (sd_pos == sd_cmdlength) { DNCommand(); SDTargetPhase(); }
		break;
	case SD_OUT:
		dn_buffer[sd_pos++] = data;
		if (sd_pos == dn_length) { DNOutputDone(); SDTargetPhase(); }
		break;
	case SD_IN:
		if (++sd_pos == dn_length) SDPhase(SD_STATUS);
		break;
	case SD_STATUS: SDPhase(SD_MSGIN); break;
	case SD_MSGIN: SDPhase(SD_FREE); break;
	case SD_MSGOUT:
		if (data == 0x06 || data == 0x0c) { /* ABORT / BUS DEVICE RESET */
			if (data == 0x0c) DNReset();
			SDPhase(SD_FREE);
		} else if ((data & 0x87) == 0x80) {
			SDPhase(SD_COMMAND); /* IDENTIFY, LUN zero */
		} else {
			DNError(5);
			SDPhase(SD_STATUS);
		}
		break;
	default: break;
	}
}

/* addr bits 0..2 select a register, bit 3 carries the Mac's A9 /DACK. */
GLOBALFUNC ui5b SCSI_Access(ui5b Data, blnr WriteMem, CPTR addr)
{
	unsigned reg = addr & 7;
	uint8_t data = (uint8_t)Data, mac[6];
	int match = sd_phase < SD_FREE && (sd_tcr & 7) == sd_phase;
	int request = sd_phase < SD_FREE && !sd_ack && !(sd_icr & 0x80);
	if ((addr & 8) && sd_dma && (sd_mode & 2) && match && request) {
		if ((WriteMem && !(sd_phase & 1)) || (!WriteMem && (sd_phase & 1))) {
			if (!WriteMem) data = SDData();
			SDByteDone(data);
			return data;
		}
	}
	if (WriteMem) {
		switch (reg) {
		case 0: sd_odr = data; break;
		case 1:
			if (data & 0x80) {
				SCSI_Reset();
				sd_icr = data;
				sd_irq = 1;
				break;
			}
			if ((data & 0x10) && !(sd_icr & 0x10) && request) {
				sd_ack = 1;
				sd_latch = sd_odr;
			}
			if (!(data & 0x10) && sd_ack) {
				sd_ack = 0;
				SDByteDone(sd_latch);
			}
			sd_icr = data;
			break;
		case 2:
			sd_mode = data;
			if (!(data & 2)) { sd_dma = sd_enddma = 0; }
			break;
		case 3: sd_tcr = data & 7; break;
		case 5: case 7:
			if (sd_mode & 2) { sd_dma = 1; sd_enddma = 0; }
			break;
		default: break;
		}
		if (!(sd_icr & 0x80)) {
			if (sd_phase == SD_FREE && (sd_icr & 0x04) && !(sd_icr & 0x08)
				&& (sd_odr & 0x7f) == 0x10 && DaynaHostGetMAC(mac)) {
				SDPhase(SD_SELECTED);
			}
			if (sd_phase == SD_SELECTED && !(sd_icr & 0x04)) {
				SDPhase((sd_icr & 0x02) ? SD_MSGOUT : SD_COMMAND);
			}
		}
		return Data;
	}
	switch (reg) {
	case 0: case 6: return SDData();
	case 1: return (sd_icr & 0x9f) | ((sd_mode & 1) ? 0x40 : 0);
	case 2: return sd_mode;
	case 3: return sd_tcr;
	case 4:
		return (sd_icr & 0x80) | ((sd_icr & 4) ? 2 : 0)
			| ((sd_phase != SD_FREE || (sd_icr & 8) || (sd_mode & 1)) ? 0x40 : 0)
			| (sd_phase < SD_FREE ? sd_phase << 2 : 0) | (request ? 0x20 : 0);
	case 5:
		return (sd_enddma ? 0x80 : 0) | ((sd_dma && match && request) ? 0x40 : 0)
			| (sd_irq ? 0x10 : 0) | (match ? 8 : 0)
			| ((sd_icr & 0x10) ? 1 : 0) | ((sd_icr & 2) ? 2 : 0);
	case 7: sd_irq = 0; return 0;
	default: return 0;
	}
}
