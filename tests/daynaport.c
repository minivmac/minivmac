/* Register-level tests against the production controller with a fake wire.
   SPDX-License-Identifier: GPL-2.0-only */
#include "PICOMMON.h"
#include "SCSIEMDV.h"
#include "DAYNAAPI.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const uint8_t mac[6] = { 2, 0, 0, 0, 0, 4 };
static uint8_t received[1514], sent[1514];
static size_t received_size, sent_size;
static int available = 1, send_ok = 1, sent_count;

int DaynaHostGetMAC(uint8_t out[6])
{
	memcpy(out, mac, 6);
	return available;
}
int DaynaHostSend(const uint8_t *frame, size_t length)
{
	assert(length <= sizeof(sent));
	memcpy(sent, frame, length);
	sent_size = length;
	++sent_count;
	return send_ok;
}
size_t DaynaHostReceive(uint8_t *frame, size_t capacity)
{
	size_t n = received_size;
	assert(n <= capacity);
	memcpy(frame, received, n);
	received_size = 0;
	return n;
}
static uint8_t rd(unsigned reg) { return (uint8_t)SCSI_Access(0, 0, reg); }
static void wr(unsigned reg, uint8_t value) { (void)SCSI_Access(value, 1, reg); }
static void phase(unsigned p)
{
	assert((rd(4) & 0x7c) == (0x60 | p << 2));
	wr(3, (uint8_t)p);
	assert(rd(5) & 8);
}
static void put(uint8_t b)
{
	wr(0, b);
	wr(1, 0x11); /* Assert data and ACK. */
	assert(!(rd(4) & 0x20));
	wr(1, 1);
}
static uint8_t get(void)
{
	uint8_t b = rd(0);
	assert(rd(0) == b); /* Peeking must not consume a byte. */
	wr(1, 0x10);
	assert(!(rd(4) & 0x20));
	wr(1, 0);
	return b;
}
static void select_target(void)
{
	assert(!(rd(4) & 0x40));
	wr(0, 0x80);
	wr(2, 1);
	assert(rd(1) & 0x40); /* Arbitration in progress. */
	assert(!(rd(1) & 0x20));
	wr(1, 0x0d); /* SEL, BSY, data bus */
	wr(0, 0x90);
	wr(2, 0);
	wr(1, 0x05); /* Release initiator BSY. */
	assert(rd(4) & 0x40);
	wr(1, 0);
	phase(2);
}
static void command(uint8_t op, unsigned length, uint8_t flags)
{
	select_target();
	put(op); put(0); put(0); put((uint8_t)(length >> 8));
	put((uint8_t)length); put(flags);
}
static void finish(uint8_t status)
{
	wr(2, 0);
	phase(3);
	assert(get() == status);
	phase(7);
	assert(get() == 0);
	assert(!(rd(4) & 0x60));
}
static void input(uint8_t *out, size_t n, int dma)
{
	size_t i;
	phase(1);
	if (dma) { wr(2, 2); wr(7, 0); }
	for (i = 0; i < n; ++i) {
		if (dma) assert(rd(5) & 0x40);
		out[i] = dma ? rd(14) : get();
	}
	if (dma) {
		assert(!(rd(5) & 0x40));
		assert((rd(5) & 0x90) == 0x90);
		(void)rd(7);
		assert(!(rd(5) & 0x10));
	}
}
static void output(const uint8_t *in, size_t n, int dma)
{
	size_t i;
	phase(0);
	if (dma) { wr(2, 2); wr(5, 0); }
	for (i = 0; i < n; ++i) {
		if (dma) { assert(rd(5) & 0x40); wr(8, in[i]); }
		else put(in[i]);
	}
}
int main(void)
{
	uint8_t bytes[1524], frame[1514], records[136];
	unsigned i, mode;
	uint32_t random = 42;
	memset(frame, 0xa5, sizeof(frame));
	SCSI_Reset();
	/* Missing target IDs and unavailable backend never assert target BSY. */
	for (i = 0; i < 7; ++i) if (i != 4) {
		wr(0, (uint8_t)(0x80 | 1 << i)); wr(1, 5);
		assert(!(rd(4) & 0x40)); wr(1, 0);
	}
	available = 0;
	wr(0, 0x90); wr(1, 5); assert(!(rd(4) & 0x40)); wr(1, 0);
	available = 1;
	for (mode = 0; mode < 2; ++mode) {
		command(0x12, 37, 0);
		input(bytes, 37, mode);
		assert(bytes[0] == 3 && bytes[2] == 1);
		assert(!memcmp(bytes + 8, "Dayna   SCSI/Link       2.0f", 28));
		finish(0);
	}
	command(0x12, 5, 0); input(bytes, 5, 0); finish(0);
	command(0x12, 0, 0); finish(0);
	command(0x00, 0, 0); finish(0);
	command(0x1a, 0, 0); finish(0);
	command(0x80, 0, 0); finish(0);
	command(0x09, 6, 0); input(bytes, 18, 0);
	assert(!memcmp(bytes, mac, 6)); finish(0);
	command(0x08, 1, 0); finish(2);
	command(0x03, 0, 0); input(bytes, 9, 0);
	assert(bytes[2] == 5); finish(0);
	command(0x03, 0, 0); input(bytes, 9, 0);
	assert(bytes[2] == 0); finish(0);
	command(0x0a, 60, 0); finish(2); /* Disabled interface. */
	command(0x0e, 0, 0x80); finish(0);
	command(0x0c, 0, 0x80); finish(0); /* Normal receive mode. */
	command(0x08, 1524, 0xc0); input(bytes, 6, 0);
	for (i = 0; i < 6; ++i) assert(bytes[i] == 0);
	finish(0);
	for (mode = 0; mode < 2; ++mode) {
		/* Known CRC-32 for sixty zero bytes: 0x04128908. */
		memset(received, 0, 60); received_size = 60;
		command(0x08, 1524, mode ? 0xc0 : 0x80);
		input(bytes, 70, mode);
		assert(bytes[0] == 0 && bytes[1] == 64 && bytes[5] == 0);
		assert(!memcmp(bytes + 66, "\x08\x89\x12\x04", 4));
		finish(0);
		command(0x0a, 60, 0); output(frame, 60, mode); finish(0);
		assert(sent_size == 60 && !memcmp(sent, frame, 60));
	}
	/* A malformed host frame is discarded and represented as an empty record. */
	memset(received, 0, sizeof(received)); received_size = 13;
	command(0x08, 1524, 0xc0); input(bytes, 6, 0); finish(0);
	for (i = 0; i < 6; ++i) assert(bytes[i] == 0);
	memcpy(received, frame, sizeof(frame)); received_size = sizeof(frame);
	command(0x08, 1524, 0xc0); input(bytes, 1524, 1);
	assert(bytes[0] == 5 && bytes[1] == 0xee);
	assert(!memcmp(bytes + 6, frame, sizeof(frame))); finish(0);
	/* Header-framed multi-packet WRITE, followed by a zero record. */
	memset(records, 0, sizeof(records));
	records[1] = records[65] = 60;
	memcpy(records + 4, frame, 60); memcpy(records + 68, frame, 60);
	i = sent_count;
	command(0x0a, 132, 0x80); output(records, 132, 1); finish(0);
	assert(sent_count == (int)i + 2);
	command(0x0a, 1515, 0); finish(2);
	command(0x0a, 60, 0x80);
	memset(bytes, 0, 4); bytes[0] = 0xff; bytes[1] = 0xff;
	output(bytes, 4, 0); finish(2);
	send_ok = 0;
	command(0x0a, 60, 0); output(frame, 60, 0); finish(2); send_ok = 1;
	command(0x0d, 6, 0); output(mac, 6, 0); finish(0);
	command(0x0d, 65535, 0); finish(2);
	command(0x0c, 4, 0x40); finish(2); /* Unsupported MAC override. */
	/* Reset aborts a partial transfer and disables the interface. */
	command(0x0a, 60, 0); phase(0); put(1);
	wr(1, 0x80); assert(rd(4) & 0x80); wr(1, 0);
	assert(!(rd(4) & 0x60));
	command(0x0a, 60, 0); finish(2);
	/* IDENTIFY before a command, then ABORT in MESSAGE OUT. */
	wr(0, 0x90); wr(1, 7); wr(1, 2); phase(6); put(0x80);
	phase(2); for (i = 0; i < 6; ++i) put(0); finish(0);
	wr(0, 0x90); wr(1, 7); wr(1, 2); phase(6); put(0x06);
	assert(!(rd(4) & 0x60));
	/* Deterministic malformed register traffic exercises bounds under ASan. */
	for (i = 0; i < 100000; ++i) {
		random = random * UINT32_C(1664525) + UINT32_C(1013904223);
		(void)SCSI_Access(random >> 24, (random >> 8) & 1, random & 15);
		if (!(i % 1000)) SCSI_Reset();
	}
	puts("DaynaPORT register, framing, reset and malformed-input tests passed");
	return 0;
}
