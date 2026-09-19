/* Optional live backend test. Does not require a ROM or guest disk.
   SPDX-License-Identifier: GPL-2.0-only */
#include "NETVMNET.h"
#include <unistd.h>

int main(int argc, char **argv)
{
	uint8_t mac[6], frame[1514], arp[60] = {0};
	unsigned attempt;
	size_t n;
	int live = argc == 2 && !strcmp(argv[1], "--shared");
	setenv("MINIVMAC_DAYNAPORT", live ? "shared" : "off", 1);
	DaynaHostInit();
	if (!live) {
		static const uint8_t offline_mac[6] = { 2, 0x4d, 0x56, 0x4d, 0, 4 };
		int ok = DaynaHostGetMAC(mac) && !memcmp(mac, offline_mac, sizeof(mac))
			&& !DaynaHostReceive(frame, sizeof(frame))
			&& !DaynaHostSend(arp, sizeof(arp));
		DaynaHostUninit();
		puts(ok ? "Disabled vmnet backend passed" : "Disabled backend failed");
		return ok ? 0 : 1;
	}
	if (!DaynaHostSharedIsReady() || !DaynaHostGetMAC(mac)) { DaynaHostUninit(); return 77; }
	/* ARP request from the reserved static guest address to the gateway. */
	memset(arp, 0xff, 6);
	memcpy(arp + 6, mac, 6);
	arp[12] = 8; arp[13] = 6;
	arp[15] = 1; arp[16] = 8; arp[18] = 6; arp[19] = 4; arp[21] = 1;
	memcpy(arp + 22, mac, 6);
	memcpy(arp + 28, "\xc0\xa8\x40\xc8", 4);
	memcpy(arp + 38, "\xc0\xa8\x40\x01", 4);
	if (!DaynaHostSend(arp, sizeof(arp))) { DaynaHostUninit(); return 1; }
	for (attempt = 0; attempt < 200; ++attempt) {
		n = DaynaHostReceive(frame, sizeof(frame));
		if (n >= 42 && frame[12] == 8 && frame[13] == 6
			&& frame[20] == 0 && frame[21] == 2
			&& !memcmp(frame + 28, arp + 38, 4)
			&& !memcmp(frame + 32, mac, 6)) {
			DaynaHostUninit();
			puts("Live vmnet gateway ARP round trip passed");
			return 0;
		}
		usleep(10000);
	}
	DaynaHostUninit();
	fputs("No gateway ARP reply received\n", stderr);
	return 1;
}
