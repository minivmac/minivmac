/* DaynaPORT host boundary. Ethernet frames exclude the FCS here.
   SPDX-License-Identifier: GPL-2.0-only */
#ifndef DAYNAAPI_H
#define DAYNAAPI_H
#include <stddef.h>
#include <stdint.h>

/* All calls originate on the emulation thread. Receive never waits.
   GetMAC supplies a stable locally-administered address while the host
   backend is unavailable, allowing the guest driver to load normally. */
int DaynaHostGetMAC(uint8_t mac[6]);
int DaynaHostSend(const uint8_t *frame, size_t length);
size_t DaynaHostReceive(uint8_t *frame, size_t capacity);

/* These control the macOS shared backend only. Guest MacTCP settings are
   unchanged; restart the guest after changing the link state. */
int DaynaHostEnableShared(void);
void DaynaHostDisable(void);
int DaynaHostSharedIsReady(void);
void DaynaHostGetStats(uint8_t mac[6], unsigned long *sent,
	unsigned long *received, unsigned long *dropped);
#endif
