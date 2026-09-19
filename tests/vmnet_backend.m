/* Deterministic tests of production backend lifecycle and packet boundaries.
   vmnet is substituted, while XPC and dispatch use the real macOS runtime.
   SPDX-License-Identifier: GPL-2.0-only */
#include <vmnet/vmnet.h>
#include <sys/uio.h>
#include <Block.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
	dispatch_queue_t queue;
	vmnet_start_interface_completion_handler_t completion;
	char mac[18];
} FakeInterface;
static FakeInterface *interfaces[32];
static unsigned starts, stops;
static int delayed, null_start, fail_start, fail_stop, fail_write, empty_write;
static size_t incoming, written;
static uint8_t wire[1514];

static void complete_on_queue(FakeInterface *f, int fail)
{
	xpc_object_t params = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_string(params, vmnet_mac_address_key, f->mac);
	xpc_dictionary_set_uint64(params, vmnet_max_packet_size_key, 1514);
	f->completion(fail ? VMNET_FAILURE : VMNET_SUCCESS, params);
	xpc_release(params);
	Block_release(f->completion);
	f->completion = NULL;
}

static void complete_start(FakeInterface *f, int fail)
{
	dispatch_sync(f->queue, ^{ complete_on_queue(f, fail); });
}

static interface_ref fake_start(xpc_object_t desc, dispatch_queue_t queue,
	vmnet_start_interface_completion_handler_t completion)
{
	FakeInterface *f;
	assert(xpc_dictionary_get_uint64(desc, vmnet_operation_mode_key) == VMNET_SHARED_MODE);
	if (null_start) return NULL;
	assert(starts < 32);
	f = calloc(1, sizeof(*f));
	assert(f);
	f->queue = queue;
	dispatch_retain(queue);
	f->completion = Block_copy(completion);
	snprintf(f->mac, sizeof(f->mac), "02:01:02:03:04:%02x", starts);
	interfaces[starts++] = f;
	if (!delayed) {
		int fail = fail_start;
		dispatch_async(queue, ^{ complete_on_queue(f, fail); });
	}
	return (interface_ref)f;
}

static vmnet_return_t fake_stop(interface_ref interface, dispatch_queue_t queue,
	vmnet_interface_completion_handler_t completion)
{
	assert(interface);
	++stops;
	if (fail_stop) return VMNET_FAILURE;
	dispatch_async(queue, ^{ completion(VMNET_SUCCESS); });
	return VMNET_SUCCESS;
}

static vmnet_return_t fake_write(interface_ref interface, struct vmpktdesc *packets, int *count)
{
	assert(interface && *count == 1);
	written = packets->vm_pkt_size;
	assert(written <= sizeof(wire));
	memcpy(wire, packets->vm_pkt_iov[0].iov_base, written);
	if (empty_write) *count = 0;
	return fail_write ? VMNET_FAILURE : VMNET_SUCCESS;
}

static vmnet_return_t fake_read(interface_ref interface, struct vmpktdesc *packets, int *count)
{
	size_t n = incoming;
	assert(interface && *count == 1);
	incoming = 0;
	if (!n) { *count = 0; return VMNET_SUCCESS; }
	/* Also simulate a bad returned length without overrunning the supplied buffer. */
	memset(packets->vm_pkt_iov[0].iov_base, 0xa5,
		n < packets->vm_pkt_size ? n : packets->vm_pkt_size);
	packets->vm_pkt_size = n;
	return VMNET_SUCCESS;
}

#define vmnet_start_interface fake_start
#define vmnet_stop_interface fake_stop
#define vmnet_write fake_write
#define vmnet_read fake_read
#define DAYNA_START_TIMEOUT_NS (100 * NSEC_PER_MSEC)
#include "NETVMNET.h"

static void check_stats(unsigned long tx, unsigned long rx, unsigned long drops)
{
	uint8_t mac[6];
	unsigned long sent, received, dropped;
	DaynaHostGetStats(mac, &sent, &received, &dropped);
	assert(sent == tx && received == rx && dropped == drops);
}

int main(void)
{
	uint8_t frame[2000], mac[6], active_mac[6];
	unsigned i;
	setenv("MINIVMAC_DAYNAPORT", "off", 1);
	DaynaHostInit();
	assert(!DaynaHostSharedIsReady());
	assert(DaynaHostGetMAC(mac)); /* Device identity is not link readiness. */
	check_stats(0, 0, 0);
	DaynaHostDisable();
	null_start = 1;
	assert(!DaynaHostEnableShared());
	null_start = 0;
	fail_start = 1;
	assert(!DaynaHostEnableShared());
	assert(!DaynaHostSharedIsReady());
	fail_start = 0;

	/* Late success from a timed-out, detached session must not reset the
	   replacement session's counters or touch its buffer. */
	delayed = 1;
	assert(!DaynaHostEnableShared());
	assert(!DaynaHostSharedIsReady());
	delayed = 0;
	assert(DaynaHostEnableShared());
	assert(DaynaHostEnableShared()); /* Idempotent enable. */
	assert(starts == 3);
	memset(frame, 0x42, sizeof(frame));
	assert(!DaynaHostSend(frame, 13));
	assert(!DaynaHostSend(frame, 1515));
	assert(DaynaHostSend(frame, 14));
	assert(written == 60 && !memcmp(wire, frame, 14));
	for (i = 14; i < 60; ++i) assert(wire[i] == 0);
	check_stats(1, 0, 0);
	assert(DaynaHostGetMAC(active_mac));
	complete_start(interfaces[1], 0);
	assert(DaynaHostGetMAC(mac) && !memcmp(mac, active_mac, 6));
	check_stats(1, 0, 0);
	assert(DaynaHostSharedIsReady());
	fail_write = 1;
	assert(!DaynaHostSend(frame, 60));
	fail_write = 0;
	empty_write = 1;
	assert(!DaynaHostSend(frame, 60));
	empty_write = 0;
	assert(!DaynaHostReceive(frame, sizeof(frame)));
	incoming = 1514;
	assert(DaynaHostReceive(frame, sizeof(frame)) == 1514);
	for (i = 0; i < 1514; ++i) assert(frame[i] == 0xa5);
	incoming = 13;
	assert(!DaynaHostReceive(frame, sizeof(frame)));
	incoming = 60;
	assert(!DaynaHostReceive(frame, 14));
	incoming = 1600;
	assert(!DaynaHostReceive(frame, sizeof(frame)));
	check_stats(1, 1, 3);
	DaynaHostDisable();
	DaynaHostDisable();
	assert(!DaynaHostSharedIsReady());
	assert(!DaynaHostSend(frame, 60));
	check_stats(0, 0, 0);

	/* Late completion after shutdown, without a replacement. */
	delayed = 1;
	assert(!DaynaHostEnableShared());
	DaynaHostDisable();
	complete_start(interfaces[3], 0);
	assert(!DaynaHostSharedIsReady());
	delayed = 0;
	assert(DaynaHostEnableShared());
	fail_stop = 1;
	DaynaHostDisable();
	fail_stop = 0;
	assert(stops == starts);
	for (i = 0; i < starts; ++i) {
		assert(!interfaces[i]->completion);
		dispatch_sync(interfaces[i]->queue, ^{});
		dispatch_release(interfaces[i]->queue);
		free(interfaces[i]);
	}
	puts("vmnet lifecycle, timeout/retry, padding, bounds and counters passed");
	return 0;
}
