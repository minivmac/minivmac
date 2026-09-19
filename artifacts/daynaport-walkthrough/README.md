# DaynaPORT walkthrough evidence

These captures document the macOS host and System 7 guest setup for the
experimental DaynaPORT SCSI/Link feature.

| Order | Capture | What it demonstrates |
| --- | --- | --- |
| 01 | `01-vmnet-gateway-arp.txt` | A privileged `vmnet` interface starts and answers gateway ARP. |
| 02 | `02-daynaport-753-installer-disk.png` | The genuine DaynaPORT 7.5.3 disk is mounted in the System 7 guest. |
| 03 | `03-driver-detected.png` | The installer offers DaynaPORT SCSI/Link 1.2.2. |
| 04 | `04-custom-scsilink-only.png` | The SCSI/Link driver is selected for installation. |
| 05 | `05-mactcp-ping-tool.png` | The System 7 startup disk contains the MacTCP Ping test application. |
| 06 | `06-mactcp-static-configuration.png` | MacTCP manual address, mask, gateway, and default DNS are configured. |
| 07 | `07-ping-google-dns-success.png` | MacTCP Ping resolves `google.com` and receives five Internet ICMP replies. |
| 08 | `08-flynn-telehack-starwars.png` | Flynn maintains a Telnet session to `telehack.com` and renders streamed terminal output. |
| 09 | `09-macweb-wikipedia.png` | MacWeb fetches FrogFind’s live Wikipedia results through the configured HTTP proxy. |
| 10 | `10-control-mode-daynaport-status.png` | The Control Mode overlay reports the active shared route, adapter MAC, and live frame counters while MacTCP Ping is running. |

Guest captures use the Mini vMac window directly. Host verification is saved
as command output so the artifact contains no unrelated desktop content.
