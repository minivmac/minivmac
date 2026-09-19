# Experimental DaynaPORT networking

This adds an optional DaynaPORT SCSI/Link adapter at SCSI ID 4 to the
Macintosh Plus emulation. The initial target is an Apple Silicon macOS
host with a 4 MB Mac Plus running System 7, MacTCP, and the original
DaynaPORT SCSI/Link driver. Networking is disabled by default.

Status: verified on the development Mac. The original DaynaPORT 7.5.3
installer detects the emulated adapter and installs **SCSI/Link 1.2.2**. A
Mac Plus running System 7.0.1 with MacTCP 2.0.6 can use the shared vmnet
network through the macOS host; MacTCP Ping resolved `google.com` and received five Internet replies after
static configuration. The feature remains experimental until it has been
reproduced on a clean second host and reviewed upstream.

## Build and run

Requires Apple's Command Line Tools and an Apple Silicon Mac:

```sh
sh build_daynaport.sh
sh tests/run_daynaport.sh
sh tests/run_vmnet_backend.sh
sh run_daynaport.sh
```

The build option is `-daynaport`, currently restricted to
`-t mcar -m Plus -api cco`. Omit it for the original SCSI implementation.
The build script regenerates the usual `cfg/`, `bld/`, Makefile and
`minivmac.app` outputs. To connect at startup, set `MINIVMAC_DAYNAPORT=shared`;
unset it or use `off` to start with the host link disabled. Control Mode can
also enable and disable the host link at runtime.

Put your Mac Plus ROM (`vMac.ROM`) and System 7 startup disk in the usual
Mini vMac locations. Disk images still use Mini vMac's existing disk
driver; no `NE4.hda` placeholder is necessary.

The shared backend uses Apple's vmnet framework. Unsigned development
builds generally require root; an appropriately entitled application can
use vmnet without root. Startup logs report errors and leave the host link
unavailable if the backend cannot start. The SCSI target still presents a
stable local MAC so the guest extension can load and report a link-down
adapter. `run_daynaport.sh` is the normal launcher for this experimental build. It
uses `sudo -n`, so sudo authorization must already be available (for example,
a cached credential or an applicable NOPASSWD rule). An entitled build can be
launched directly with `MINIVMAC_DAYNAPORT=shared`, without this sudo wrapper.
To launch an unsigned binary with a normal sudo password prompt instead:

```sh
sudo env MINIVMAC_DAYNAPORT=shared ./minivmac.app/Contents/MacOS/minivmac
```

That command runs the whole emulator as root and may create root-owned files.
A privileged networking helper is a follow-up before routine use.
This implementation does not install a helper or request Wi-Fi
credentials; vmnet creates and manages the shared network.
The configured subnet must not conflict with another network on the host.

## Control Mode

Hold Control and press `N` for adapter status, `E` to enable shared networking,
or `D` to disable it. Enabling requires the same vmnet privilege as startup.
After enabling or re-enabling, restart the guest so its driver reads the new
vmnet-assigned MAC address.

Status shows the adapter MAC, suggested MacTCP address and router, successful
host send/receive counts, and invalid or oversized receive frames discarded by
the backend. It cannot read or change the guest's MacTCP settings. Counters
reset with each new shared interface; they do not measure all packet loss.

## Guest setup

1. Boot System 7 with 4 MB RAM.
2. Install MacTCP and the DaynaPORT 7.5.3 **SCSI/Link** driver. Use the
   installer's custom selection if its bundled Apple networking files
   are older than those in the guest. Restart.
3. In MacTCP choose Ethernet Built-In, manual addressing, and configure:

   | Setting | Value |
   | --- | --- |
   | IP address | `192.168.64.200` |
   | Subnet mask | `255.255.255.0` |
   | Router | `192.168.64.1` |
   | DNS | A reachable DNS server, for example `1.1.1.1` |

4. Restart and use MacTCP Ping to test the gateway, an Internet IP,
   and then a DNS name. Finally test a TCP service supported by the guest.

The `.2`–`.100` addresses are reserved for vmnet DHCP; `.200` is outside
that pool for MacTCP's static configuration. Only one guest should use
that static address. Shared networking provides outbound IP access;
it does not bridge EtherTalk onto the physical LAN.

## Implementation and limits

- `SCSIDNPT.h`: single-target NCR5380 selection, arbitration indication,
  REQ/ACK handshaking, polled and A9 pseudo-DMA transfers, phase status,
  IDENTIFY/ABORT, bus reset, and command completion. Included by
  `SCSIEMDV.c` only in enabled builds. No disconnect/reselect, target
  mode, parity simulation, or interrupt-driven SCSI is implemented.
- `DAYNAPRT.h`: inquiry, sense, MAC/statistics, interface enable/disable,
  packet reads, direct and record-framed writes. Receive records contain
  padded Ethernet frames and a computed FCS. Each READ returns at most
  one packet. Invalid/oversize requests return CHECK CONDITION.
- `DAYNAAPI.h`: host interface exchanging Ethernet frames without FCS.
- `NETVMNET.h`: macOS shared network, vmnet-assigned MAC, serialized
  access, bounded receive buffers and startup timeout. The framework
  owns the pending receive queue; the guest polls it with READ commands.

MAC overrides return CHECK CONDITION because vmnet assigns the address.
Receive-mode and multicast configuration commands are accepted, but no
hardware multicast filter is emulated. BlueSCSI's Wi-Fi management
extensions are not implemented. MAC/statistics reports zero hardware
error counters. Guest timing compatibility, especially blind transfers,
still needs confirmation against the real driver.

## Verification

`tests/run_daynaport.sh` compiles the production SCSI controller with a
fake Ethernet backend under ASan and UBSan. It covers target selection,
inquiry, short allocations, sense, both transfer paths, empty and maximum
packet reads, FCS, framed multi-packet writes, invalid sizes, backend
failure, reset during transfer, and malformed register traffic.

`sh tests/run_vmnet_backend.sh` tests the production macOS backend under
ASan and UBSan with substituted vmnet calls and real dispatch/XPC. No ROM or
privilege is needed. It covers failed and timed-out starts, late completion
after disable/re-enable, repeated enable/disable, packet padding, receive
bounds, counters, and a rejected stop request.

An optional backend-only smoke test needs no ROM:

```sh
cc -Wall -Wextra -Werror -fblocks -Isrc tests/vmnet_smoke.m \
  -framework Foundation -framework vmnet -o bld/vmnet-smoke
./bld/vmnet-smoke
sudo ./bld/vmnet-smoke --shared
```

The first invocation checks that a disabled host link stays inactive. The
second opens a real shared interface and checks a gateway ARP round trip;
exit 77 means the interface could not start. Neither test proves guest
driver compatibility or end-to-end Internet access.

Before proposing this upstream, run the following matrix on a clean checkout:

1. Build the ordinary Mac Plus configuration, then run
   `sh build_daynaport.sh`. The DaynaPORT script forces a complete rebuild so
   configuration-specific SCSI objects cannot be reused from the ordinary
   build.
2. Run `sh tests/run_daynaport.sh` and `sh tests/run_vmnet_backend.sh`;
   these use ASan and UBSan against the production SCSI target and host backend
   with deterministic substitutes for their external dependencies.
3. Build `tests/vmnet_smoke.m`, run it with networking disabled, and run its
   `--shared` form with the required vmnet privilege. Preserve its gateway-ARP
   output as a host-side artifact.
4. Boot a fresh System 7 disk with the original DaynaPORT installer and
   MacTCP, install the SCSI/Link driver, enable EtherTalk, configure MacTCP,
   and reboot. Capture each state.
5. From the guest, ping `192.168.64.1`, then an external IP such as
   `1.1.1.1`, then a DNS name. Finally make one outbound TCP connection using
   a period-appropriate client. Repeat this guest test on a second macOS host.

The first three tests are automated; the final two verify the original guest
driver, vmnet entitlement behavior, DNS, and traffic outside the local shared
network.

Verification on the development host: the default and networking-enabled
builds passed; sanitizer tests and the disabled and privileged shared-backend
smoke tests passed. An unsigned non-root launch returned vmnet status 1001.
A privileged launch started the shared interface and completed a gateway ARP
round trip. In the Mac Plus/System 7.0.1 guest, the original DaynaPORT 7.5.3
installer detected SCSI/Link 1.2.2, EtherTalk switched without error, and
MacTCP 2.0.6 resolved `google.com` and received five Internet ICMP replies
through the static shared-network configuration. Flynn Lite also completed a
Telnet session to `telehack.com`, exercising outbound TCP and sustained data
exchange. MacWeb 2.0c+ also retrieved FrogFind Wikipedia search results through
the configured HTTP proxy.

## References

- [BlueSCSI WiFi DaynaPORT setup](https://bluescsi.com/docs/WiFi-DaynaPORT)
- [BlueSCSI DaynaPORT protocol implementation](https://github.com/BlueSCSI/BlueSCSI-v2/blob/92db68f2d9b480cbb3b369a471ec55a9758da818/lib/SCSI2SD/src/firmware/network.c)
- [DaynaPort command reference (Roger Burrows / PiSCSI)](https://github.com/PiSCSI/piscsi/wiki/Dayna-Port-Command-Set)
- [Apple vmnet framework](https://developer.apple.com/documentation/vmnet)

Starting Mini vMac revision: `6860a9a7f372c461b25dc42974b191a453ebcfb2`.
The new code is GPL-2.0-only, like Mini vMac. BlueSCSI and the protocol
documentation were used as behavioral references; their firmware is not
vendored into this emulator.
