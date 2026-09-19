/* macOS shared Ethernet backend, included by OSGLUCCO.m.
   SPDX-License-Identifier: GPL-2.0-only */
#include "DAYNAAPI.h"
#include <vmnet/vmnet.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Each start owns its callback state. A timed-out start may complete after
   disable/re-enable, so callbacks must never access a replacement session. */
typedef struct {
	interface_ref interface;
	dispatch_queue_t queue;
	unsigned references; /* Accessed only on queue once callbacks can run. */
	int ready, closing;
	uint8_t mac[6];
	unsigned long sent, received, dropped;
	uint8_t *receive_buffer;
	size_t receive_capacity;
} DaynaSession;
static DaynaSession *dn_session;
/* A SCSI/Link card remains present when its host Ethernet is unavailable. */
static const uint8_t dn_offline_mac[6] = { 0x02, 0x4d, 0x56, 0x4d, 0x00, 0x04 };

#ifndef DAYNA_START_TIMEOUT_NS
#define DAYNA_START_TIMEOUT_NS (10 * NSEC_PER_SEC)
#endif

/* Called on the session queue. The owner and pending startup callback each
   hold a reference; the last one releases the session and its queue. */
static void DaynaSessionRelease(DaynaSession *s)
{
	if (!--s->references) {
		dispatch_release(s->queue);
		free(s->receive_buffer);
		free(s);
	}
}

static void DaynaHostUninit(void);

static int DaynaHostStartShared(void)
{
	xpc_object_t desc;
	dispatch_semaphore_t done;
	__block int ready;
	DaynaSession *s = calloc(1, sizeof(*s));
	if (!s) return 0;
	s->queue = dispatch_queue_create("org.minivmac.daynaport", DISPATCH_QUEUE_SERIAL);
	done = dispatch_semaphore_create(0);
	if (!s->queue || !done) {
		if (s->queue) dispatch_release(s->queue);
		if (done) dispatch_release(done);
		free(s);
		return 0;
	}
	s->references = 2;
	dn_session = s;
	/* The callback owns a reference even when startup times out. */
	dispatch_retain(done);
	desc = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_uint64(desc, vmnet_operation_mode_key, VMNET_SHARED_MODE);
	/* Reserve .200 for MacTCP, outside the .2-.100 DHCP pool. */
	xpc_dictionary_set_string(desc, vmnet_start_address_key, "192.168.64.1");
	xpc_dictionary_set_string(desc, vmnet_end_address_key, "192.168.64.100");
	xpc_dictionary_set_string(desc, vmnet_subnet_mask_key, "255.255.255.0");
	s->interface = vmnet_start_interface(desc, s->queue,
		^(vmnet_return_t status, xpc_object_t params) {
			if (!s->closing && status == VMNET_SUCCESS && params) {
				unsigned m[6];
				unsigned i;
				const char *mac = xpc_dictionary_get_string(params, vmnet_mac_address_key);
				uint64_t capacity = xpc_dictionary_get_uint64(params, vmnet_max_packet_size_key);
				if (mac && sscanf(mac, "%x:%x:%x:%x:%x:%x",
					&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) == 6
					&& capacity >= 1514 && capacity <= 65536) {
					for (i = 0; i < 6; ++i) s->mac[i] = (uint8_t)m[i];
					s->receive_capacity = (size_t)capacity;
					s->receive_buffer = malloc(s->receive_capacity);
					s->ready = s->receive_buffer != NULL;
				}
				if (s->ready) fprintf(stderr,
					"DaynaPORT: SCSI ID 4, MAC %s, shared network ready.\n"
					"MacTCP: IP 192.168.64.200, mask 255.255.255.0, router 192.168.64.1.\n", mac);
			}
			if (!s->closing && !s->ready) fprintf(stderr,
				"DaynaPORT: vmnet startup failed (status %d). "
				"vmnet requires root or the com.apple.vm.networking entitlement.\n", (int)status);
			dispatch_semaphore_signal(done);
			dispatch_release(done);
			DaynaSessionRelease(s);
		});
	xpc_release(desc);
	if (s->interface) {
		if (dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, DAYNA_START_TIMEOUT_NS))) {
			fprintf(stderr, "DaynaPORT: vmnet startup timed out; adapter unavailable.\n");
			dispatch_sync(s->queue, ^{ s->closing = 1; s->ready = 0; });
		}
	} else {
		fprintf(stderr, "DaynaPORT: vmnet could not create an interface.\n");
		dispatch_release(done); /* No callback scheduled. */
		dispatch_sync(s->queue, ^{ DaynaSessionRelease(s); });
	}
	dispatch_release(done);
	dispatch_sync(s->queue, ^{ ready = s->ready && !s->closing; });
	return ready;
}

int DaynaHostEnableShared(void)
{
	DaynaSession *s = dn_session;
	__block int ready = 0;
	if (s) dispatch_sync(s->queue, ^{
		ready = s->ready && !s->closing;
	});
	if (ready) return 1;
	if (s) DaynaHostUninit();
	return DaynaHostStartShared();
}

void DaynaHostDisable(void)
{
	DaynaHostUninit();
}

int DaynaHostSharedIsReady(void)
{
	DaynaSession *s = dn_session;
	__block int ready = 0;
	if (s) dispatch_sync(s->queue, ^{
		ready = s->ready && !s->closing;
	});
	return ready;
}

void DaynaHostGetStats(uint8_t mac[6], unsigned long *sent,
	unsigned long *received, unsigned long *dropped)
{
	DaynaSession *s = dn_session;
	if (s) dispatch_sync(s->queue, ^{
		memcpy(mac, (s->ready && !s->closing) ? s->mac : dn_offline_mac, 6);
		*sent = s->sent;
		*received = s->received;
		*dropped = s->dropped;
	});
	else {
		memcpy(mac, dn_offline_mac, 6);
		*sent = *received = *dropped = 0;
	}
}

static void DaynaHostInit(void)
{
	const char *mode = getenv("MINIVMAC_DAYNAPORT");
	if (!mode || !*mode || !strcmp(mode, "off")) return;
	if (strcmp(mode, "shared")) {
		fprintf(stderr, "DaynaPORT: MINIVMAC_DAYNAPORT must be shared or off.\n");
		return;
	}
	(void)DaynaHostEnableShared();
}

static void DaynaHostUninit(void)
{
	DaynaSession *s = dn_session;
	dispatch_queue_t queue;
	if (!s) return;
	/* dispatch_sync does not retain its queue; keep it alive even if the
	   owner reference is the last session reference released below. */
	queue = s->queue;
	dispatch_retain(queue);
	dn_session = NULL;
	dispatch_sync(s->queue, ^{
		s->closing = 1;
		s->ready = 0;
		free(s->receive_buffer);
		s->receive_buffer = NULL;
		s->receive_capacity = 0;
	});
	if (s->interface) {
		vmnet_return_t result = vmnet_stop_interface(s->interface, s->queue,
			^(vmnet_return_t status) {
				if (status != VMNET_SUCCESS) fprintf(stderr,
					"DaynaPORT: vmnet stop failed (status %d).\n", (int)status);
			});
		if (result != VMNET_SUCCESS) fprintf(stderr,
			"DaynaPORT: vmnet stop request failed (status %d).\n", (int)result);
	}
	dispatch_sync(queue, ^{ DaynaSessionRelease(s); });
	dispatch_release(queue);
}

int DaynaHostGetMAC(uint8_t mac[6])
{
	DaynaSession *s = dn_session;
	__block int ready = 0;
	if (s) dispatch_sync(s->queue, ^{
		ready = s->ready && !s->closing;
		if (ready) memcpy(mac, s->mac, 6);
	});
	if (!ready) memcpy(mac, dn_offline_mac, sizeof(dn_offline_mac));
	return 1;
}

int DaynaHostSend(const uint8_t *frame, size_t length)
{
	DaynaSession *s = dn_session;
	__block int ok = 0;
	if (!s || length < 14 || length > 1514) return 0;
	dispatch_sync(s->queue, ^{
		if (s->ready && !s->closing) {
			uint8_t padded[1514];
			struct iovec iov;
			struct vmpktdesc packet;
			int count = 1;
			size_t size = length < 60 ? 60 : length;
			memset(padded, 0, size);
			memcpy(padded, frame, length);
			iov.iov_base = padded;
			iov.iov_len = size;
			memset(&packet, 0, sizeof(packet));
			packet.vm_pkt_iov = &iov;
			packet.vm_pkt_iovcnt = 1;
			packet.vm_pkt_size = size;
			ok = vmnet_write(s->interface, &packet, &count) == VMNET_SUCCESS && count == 1;
			if (ok) ++s->sent;
		}
	});
	return ok;
}

size_t DaynaHostReceive(uint8_t *frame, size_t capacity)
{
	DaynaSession *s = dn_session;
	__block size_t length = 0;
	if (!s) return 0;
	dispatch_sync(s->queue, ^{
		if (s->ready && !s->closing) {
			struct iovec iov = { s->receive_buffer, s->receive_capacity };
			struct vmpktdesc packet;
			int count = 1;
			memset(&packet, 0, sizeof(packet));
			packet.vm_pkt_iov = &iov;
			packet.vm_pkt_iovcnt = 1;
			packet.vm_pkt_size = s->receive_capacity;
			if (vmnet_read(s->interface, &packet, &count) == VMNET_SUCCESS && count == 1) {
				if (packet.vm_pkt_size >= 14 && packet.vm_pkt_size <= capacity
					&& packet.vm_pkt_size <= s->receive_capacity) {
					length = packet.vm_pkt_size;
					memcpy(frame, s->receive_buffer, length);
					++s->received;
				} else {
					++s->dropped;
				}
			}
		}
	});
	return length;
}
