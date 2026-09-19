/* macOS shared Ethernet backend, included by OSGLUCCO.m.
   SPDX-License-Identifier: GPL-2.0-only */
#include "DAYNAAPI.h"
#include <vmnet/vmnet.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static interface_ref dn_interface;
static dispatch_queue_t dn_queue;
static int dn_ready, dn_closing;
static uint8_t dn_mac[6];
static unsigned long dn_sent, dn_received, dn_dropped;
/* A SCSI/Link card remains present when its host Ethernet is unavailable. */
static const uint8_t dn_offline_mac[6] = { 0x02, 0x4d, 0x56, 0x4d, 0x00, 0x04 };
static uint8_t *dn_receive_buffer;
static size_t dn_receive_capacity;

static void DaynaHostUninit(void);

static int DaynaHostStartShared(void)
{
	xpc_object_t desc;
	dispatch_semaphore_t done;
	dn_closing = 0;
	dn_sent = dn_received = dn_dropped = 0;
	dn_queue = dispatch_queue_create("org.minivmac.daynaport", DISPATCH_QUEUE_SERIAL);
	done = dispatch_semaphore_create(0);
	/* The callback owns a reference even when startup times out. */
	dispatch_retain(done);
	desc = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_uint64(desc, vmnet_operation_mode_key, VMNET_SHARED_MODE);
	/* Reserve .200 for MacTCP, outside the .2-.100 DHCP pool. */
	xpc_dictionary_set_string(desc, vmnet_start_address_key, "192.168.64.1");
	xpc_dictionary_set_string(desc, vmnet_end_address_key, "192.168.64.100");
	xpc_dictionary_set_string(desc, vmnet_subnet_mask_key, "255.255.255.0");
	dn_interface = vmnet_start_interface(desc, dn_queue,
		^(vmnet_return_t status, xpc_object_t params) {
			if (!dn_closing && status == VMNET_SUCCESS && params) {
				unsigned m[6];
				unsigned i;
				const char *mac = xpc_dictionary_get_string(params, vmnet_mac_address_key);
				uint64_t capacity = xpc_dictionary_get_uint64(params, vmnet_max_packet_size_key);
				if (mac && sscanf(mac, "%x:%x:%x:%x:%x:%x",
					&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) == 6
					&& capacity >= 1514 && capacity <= 65536) {
					for (i = 0; i < 6; ++i) dn_mac[i] = (uint8_t)m[i];
					dn_receive_capacity = (size_t)capacity;
					dn_receive_buffer = malloc(dn_receive_capacity);
					dn_ready = dn_receive_buffer != NULL;
				}
				if (dn_ready) fprintf(stderr,
					"DaynaPORT: SCSI ID 4, MAC %s, shared network ready.\n"
					"MacTCP: IP 192.168.64.200, mask 255.255.255.0, router 192.168.64.1.\n", mac);
			}
			if (!dn_closing && !dn_ready) fprintf(stderr,
				"DaynaPORT: vmnet startup failed (status %d). "
				"vmnet requires root or the com.apple.vm.networking entitlement.\n", (int)status);
			dispatch_semaphore_signal(done);
			dispatch_release(done);
		});
	xpc_release(desc);
	if (dn_interface) {
		if (dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC))) {
			fprintf(stderr, "DaynaPORT: vmnet startup timed out; adapter unavailable.\n");
			dispatch_sync(dn_queue, ^{ dn_closing = 1; });
		}
	} else {
		fprintf(stderr, "DaynaPORT: vmnet could not create an interface.\n");
		dispatch_release(done); /* No callback scheduled. */
	}
	dispatch_release(done);
	return dn_ready;
}

int DaynaHostEnableShared(void)
{
	__block int ready = 0;
	if (dn_queue) dispatch_sync(dn_queue, ^{
		ready = dn_ready && !dn_closing;
	});
	if (ready) return 1;
	if (dn_queue) DaynaHostUninit();
	return DaynaHostStartShared();
}

void DaynaHostDisable(void)
{
	DaynaHostUninit();
}

int DaynaHostSharedIsReady(void)
{
	__block int ready = 0;
	if (dn_queue) dispatch_sync(dn_queue, ^{
		ready = dn_ready && !dn_closing;
	});
	return ready;
}

void DaynaHostGetStats(uint8_t mac[6], unsigned long *sent,
	unsigned long *received, unsigned long *dropped)
{
	if (dn_queue) dispatch_sync(dn_queue, ^{
		memcpy(mac, dn_ready ? dn_mac : dn_offline_mac, 6);
		*sent = dn_sent;
		*received = dn_received;
		*dropped = dn_dropped;
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
	if (!dn_queue) return;
	dispatch_sync(dn_queue, ^{
		dn_closing = 1;
		dn_ready = 0;
		free(dn_receive_buffer);
		dn_receive_buffer = NULL;
	});
	if (dn_interface) {
		vmnet_stop_interface(dn_interface, dn_queue, ^(vmnet_return_t status) {
			if (status != VMNET_SUCCESS) fprintf(stderr,
				"DaynaPORT: vmnet stop failed (status %d).\n", (int)status);
		});
		dn_interface = NULL;
	}
	dispatch_release(dn_queue);
	dn_queue = NULL;
}

int DaynaHostGetMAC(uint8_t mac[6])
{
	__block int ready = 0;
	if (dn_queue) dispatch_sync(dn_queue, ^{
		ready = dn_ready && !dn_closing;
		if (ready) memcpy(mac, dn_mac, 6);
	});
	if (!ready) memcpy(mac, dn_offline_mac, sizeof(dn_offline_mac));
	return 1;
}

int DaynaHostSend(const uint8_t *frame, size_t length)
{
	__block int ok = 0;
	if (!dn_queue || length < 14 || length > 1514) return 0;
	dispatch_sync(dn_queue, ^{
		if (dn_ready && !dn_closing) {
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
			ok = vmnet_write(dn_interface, &packet, &count) == VMNET_SUCCESS && count == 1;
			if (ok) ++dn_sent;
		}
	});
	return ok;
}

size_t DaynaHostReceive(uint8_t *frame, size_t capacity)
{
	__block size_t length = 0;
	if (!dn_queue) return 0;
	dispatch_sync(dn_queue, ^{
		if (dn_ready && !dn_closing) {
			struct iovec iov = { dn_receive_buffer, dn_receive_capacity };
			struct vmpktdesc packet;
			int count = 1;
			memset(&packet, 0, sizeof(packet));
			packet.vm_pkt_iov = &iov;
			packet.vm_pkt_iovcnt = 1;
			packet.vm_pkt_size = dn_receive_capacity;
			if (vmnet_read(dn_interface, &packet, &count) == VMNET_SUCCESS && count == 1) {
				if (packet.vm_pkt_size >= 14 && packet.vm_pkt_size <= capacity) {
					length = packet.vm_pkt_size;
					memcpy(frame, dn_receive_buffer, length);
					++dn_received;
				} else {
					++dn_dropped;
				}
			}
		}
	});
	return length;
}
