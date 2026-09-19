/*
	LTOVRBPF.h

	Copyright (C) 2012 Michael Fort, Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

/*
	LocalTalk OVeR Berkeley Packet Filter
*/


#define BPF_dolog (dbglog_HAVE && 0)

/* BPF and devices */
static unsigned char device_address[6] = {
	0
};
static unsigned int device_buffer_size = 0;
static int fd = -1; /* BPF file descriptor */
static struct bpf_version bpf_version;
static struct bpf_program bpf_program;
static struct bpf_insn insns[] =
{
	/* Program for BPF to filter out non-LTOE packets */
	BPF_STMT(BPF_LD + BPF_H + BPF_ABS, 12),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, 0x809B, 0, 1),
	BPF_STMT(BPF_RET + BPF_K, 65535),
	BPF_STMT(BPF_RET + BPF_K, 0),
};

/*
	Transmit buffer that is reused from tx packet to tx packet.
	The 's' byte represents the source mac address (ours) and we don't
	have to initialize it because the MAC device will automatically
	fill it in for us.  The four 'p' bytes represent the process number
	of this Mini vMac application.  It helps differentiate packets
	between two applications running on the same machine.  It is not
	used at this time.  There is a small chance two applications could
	get the same LLAP/SDLC address to start with and would not work
	correctly (1 in 254). The 'S' bytes are the size of the LLAP packet
	since it can be smaller than the minimum sized Ethernet frame.
	The process number is replaced at initialization.  The size is
	updated just before sending to BPF.  All LLAP data is inserted
	starting at byte 20.
*/
static unsigned char tx_buffer[20 + LT_TxBfMxSz] =
	"\xFF\xFF\xFF\xFF\xFF\xFFssssss\x80\x9BppppSS";

/* Macro used by only the get_sockaddrs function for readability. */
#define ROUNDUP(a, size) \
	(((a) & ((size) - 1)) ? (1 + ((a) | ((size) - 1))) : (a))

/*
	Utility function needed for walking the addresses of the
	kernel route lookup
*/
LOCALPROC get_sockaddrs(int addrs, struct sockaddr* sa,
	struct sockaddr** rti_info)
{
	int loop;
	int incr;

	for (loop = 0; loop < RTAX_MAX; loop++) {
		if (addrs & (1 << loop)) {
			rti_info[loop] = sa;
			incr = sa->sa_len ? ROUNDUP(sa->sa_len, sizeof(uint32_t))
				: sizeof(uint32_t);
			sa = (struct sockaddr*)((unsigned long int)sa + incr);
		} else {
			rti_info[loop] = NULL;
		}
	}
}

/*
	This ugly function does a lot of steps to prepare the BPF
	for our use.
	1. Find the ethernet interface that the default route uses.
	2. Determine the maximum number of BPF devices can exist.
	3. Search for a usable BPF device and open it.
	4. Set the BPF device to use our ethernet interface.
	5. Get the proper buffer size to use with the BPF device.
	6. Set some useful modes of operation on the BPF device.
	7. Install a program on the device that filters out non-LTOE
	packets.
*/

LOCALFUNC int get_ethernet(void)
{
	int result;
	int size;
	struct rt_msghdr* message = NULL;
	struct sockaddr_in* addrs;
	struct sockaddr* sa_list[RTAX_MAX];
	struct sockaddr* sa;
	struct sockaddr_dl* link;
	int namelen;
	int loop;
	char filename[64];
	struct ifreq ifreq;
	int enable = 1;
	struct kinfo_proc kp;
	size_t len = sizeof(kp);
	int max = 4;
	int v = falseblnr;

	char device[32];

	/* Get a socket to routed for IPv4 */
	fd = socket(PF_ROUTE, SOCK_RAW, AF_INET);
	if (fd == -1) {
		goto label_exit;
	}

	/* Allocate a message */
	size = sizeof(struct rt_msghdr) + 16 * sizeof(struct sockaddr_in);
	message = (struct rt_msghdr*)malloc(size);
	if (! message) {
		goto label_exit;
	}
	memset(message, 0, size);
	addrs = (struct sockaddr_in*)(message + 1);

	/* Fill in the request */
	message->rtm_msglen    = size;
	message->rtm_version   = RTM_VERSION;
	message->rtm_type      = RTM_GET;
	message->rtm_addrs
		= RTA_DST | RTA_GATEWAY | RTA_NETMASK | RTA_IFP | RTA_IFA;
	addrs->sin_len         = sizeof(struct sockaddr_in);
	addrs->sin_family      = AF_INET;
	addrs->sin_addr.s_addr = 0; /* 0.0.0.0 is default route */

	/* Send the message to the kernel */
	result = write(fd, message, size);
	if (result < 0) {
		goto label_exit;
	}

	/* Read the result from the kernel */
	result = read(fd, message, size);
	if ((result < (int)sizeof(struct rt_msghdr))
		|| (message->rtm_msglen > result))
	{
		goto label_exit;
	}

	/* fd is shared with the BPF device opened below. */
	close(fd);
	fd = -1;

	/* Get pointer to the result then parse it */
	sa = (struct sockaddr*)
		((unsigned long int)message + sizeof(struct rt_msghdr));
	get_sockaddrs(message->rtm_addrs, sa, sa_list);

	/* Must have a LINK (Ethernet) */
	if ((! sa_list[RTAX_IFP])
		|| (sa_list[RTAX_IFP]->sa_family != AF_LINK))
	{
		goto label_exit;
	}

	link = (struct sockaddr_dl*)sa_list[RTAX_IFP];
	namelen = link->sdl_nlen;
	if ((namelen >= (int)sizeof(device)) || (link->sdl_alen < 6)) {
		goto label_exit;
	}

	strncpy(device, &link->sdl_data[0], namelen);
	device[namelen] = 0;
	memcpy(device_address, &link->sdl_data[namelen], 6);
	memcpy(&(tx_buffer[6]), &link->sdl_data[namelen], 6);

	result = sysctlbyname("debug.bpf_maxdevices", &kp, &len, NULL, 0);
	if (result == -1) {
		goto label_exit;
	}
	max = *((int *)&kp);

	for (loop = 0; loop < max; loop++) {
		sprintf(filename, "/dev/bpf%d", loop);
		fd = open(filename, O_RDWR | O_NONBLOCK | O_EXLOCK);
		if (fd >= 0) {
			/* sprintf(buffer, "using %s\n", filename); */
			break;
		}
	}

	if (fd < 0) {
		goto label_exit;
	}

	memset(&ifreq, 0, sizeof(struct ifreq));
	strncpy(ifreq.ifr_name, device, IFNAMSIZ);
	result = ioctl(fd, BIOCSETIF, &ifreq);
	if (result) {
		goto label_exit;
	}

	result = ioctl(fd, BIOCGBLEN, &device_buffer_size);
	if (result) {
		goto label_exit;
	}

	result = ioctl(fd, BIOCPROMISC, &enable);
	if (result) {
		goto label_exit;
	}

	result = ioctl(fd, BIOCSSEESENT, &enable);
	if (result) {
		goto label_exit;
	}

	result = ioctl(fd, BIOCSHDRCMPLT, &enable);
	if (result) {
		goto label_exit;
	}

	result = ioctl(fd, BIOCIMMEDIATE, &enable);
	if (result) {
		goto label_exit;
	}

	result = ioctl(fd, BIOCVERSION, &bpf_version);
	if (result) {
		goto label_exit;
	}

	bpf_program.bf_len = 4;
	bpf_program.bf_insns = insns;

	result = ioctl(fd, BIOCSETF, &bpf_program);
	if (result) {
		goto label_exit;
	}

	v = trueblnr;

label_exit:
	if (NULL != message) {
		free(message);
	}
	if ((! v) && (fd >= 0)) {
		close(fd);
		fd = -1;
	}

	return v;
}

LOCALVAR unsigned char *MyRxBuffer = NULL;

/*
	External function needed at startup to initialize the LocalTalk
	functionality.
*/
LOCALFUNC int InitLocalTalk(void)
{
	/* Failure to access BPF should leave LocalTalk inert, not prevent
	   the emulator from starting. */
	(void)get_ethernet();

	LT_PickStampNodeHint();

	/*
		Save the process id in the transmit buffer as it may be used
		later to uniquely identify the sender to identify collisions
		in dynamic llap node address assignment.
	*/
	*((uint32_t*)(&tx_buffer[14])) = htonl(LT_MyStamp /* getpid() */);

	LT_TxBuffer = (ui3p)&tx_buffer[20];

	if (0 != device_buffer_size) {
		MyRxBuffer = malloc(device_buffer_size);
		if (NULL == MyRxBuffer) {
			return falseblnr;
		}
	}

	/* Initialized properly */
	return trueblnr;
}

LOCALPROC UnInitLocalTalk(void)
{
	if (NULL != MyRxBuffer) {
		free(MyRxBuffer);
		MyRxBuffer = NULL;
	}
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
}

GLOBALOSGLUPROC LT_TransmitPacket(void)
{
	int count;

	/*
		Write the length in the packet.  This is needed because
		Ethernet has a minimum 60 bytes length, which the MAC chip
		will enforce on TX.  Without the size, a simple 3 byte LLAP
		packet would look like a (60 - 14 =) 46 byte LLAP packet.
	*/
	*((uint16_t*)(&tx_buffer[18])) = htons(LT_TxBuffSz);

	/* Send the packet to Ethernet */
	if (fd < 0) {
		return;
	}
	count = write(fd, tx_buffer, 20 + LT_TxBuffSz);

	(void)count; /* unused */
}

LOCALVAR unsigned char* NextPacket = NULL;
LOCALVAR unsigned char* EndPackets = NULL;

LOCALPROC LocalTalkTick0(void)
{
	/* Get a single buffer worth of packets from BPF */
	unsigned char* device_buffer = MyRxBuffer;
	int bytes;

	if ((fd < 0) || (NULL == device_buffer)) {
		return;
	}

	bytes = read(fd, device_buffer, device_buffer_size);
	if (bytes > 0) {
		/* Maybe multiple packets in this buffer */
#if 0
		dbglog_writeln("SCC founds packets from BPF");
#endif
		NextPacket = device_buffer;
		EndPackets = device_buffer + bytes;
	}
}

GLOBALOSGLUPROC LT_ReceivePacket(void)
{
label_retry:
	if (NextPacket == NULL) {
		LocalTalkTick0();
		if (NextPacket != NULL) {
			goto label_retry;
		}
	} else if (NextPacket >= EndPackets) {
#if 0
		dbglog_writeln("SCC finished set of packets from BPF");
#endif
		NextPacket = NULL;
		goto label_retry;
	} else {
		unsigned char* packet = NextPacket;
		unsigned int packet_size;
		int payload_length;
		int llap_length;
		unsigned char *buff;
		unsigned char* start;
		/* Get pointer to BPF header */
		struct bpf_hdr* header = (struct bpf_hdr *)packet;
		if ((unsigned long)(EndPackets - packet) < sizeof(struct bpf_hdr)) {
			NextPacket = NULL;
			goto label_retry;
		}
		if ((header->bh_hdrlen > (unsigned int)(EndPackets - packet))
			|| (header->bh_caplen > (unsigned int)(EndPackets - packet)
				- header->bh_hdrlen))
		{
			NextPacket = NULL;
			goto label_retry;
		}
		packet_size = BPF_WORDALIGN(header->bh_hdrlen + header->bh_caplen);
		if (packet_size > (unsigned int)(EndPackets - packet)) {
			NextPacket = NULL;
			goto label_retry;
		}

		/* Advance to next packet in buffer */
		NextPacket += packet_size;

		/* Get clean references to data */
		payload_length = (int)header->bh_caplen - 20;
		if (payload_length < 0) {
			goto label_retry;
		}
		buff = packet + header->bh_hdrlen;
		llap_length = ntohs(*((uint16_t*)(buff + 18)));
		start = buff + 20;

		if (llap_length <= payload_length) {
			/* Start the receiver */
			CertainlyNotMyPacket = (LT_MyStamp !=
				ntohl(*((uint32_t*)(buff + 14))));

#if BPF_dolog
			dbglog_writeCStr("LT_MyStamp: ");
			dbglog_writeNum(LT_MyStamp);
			dbglog_writeCStr(", received stamp: ");
			dbglog_writeNum(ntohl(*((uint32_t*)(buff + 14))));
			dbglog_writeCStr(", CertainlyNotMyPacket: ");
			dbglog_writeNum(CertainlyNotMyPacket);
			dbglog_writeReturn();
#endif

			LT_RxBuffer    = (ui3p)start;
			LT_RxBuffSz    = llap_length;
		} else {
			goto label_retry;
		}
	}
}

#if ! LT_MayHaveEcho
#error "LT over BPF does not implement preventing echo"
#endif
