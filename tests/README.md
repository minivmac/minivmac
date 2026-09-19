# Disk image bounds regression tests

Generate a normal Macintosh Plus configuration, then run the test on macOS:

```sh
cc -o setup_t setup/tool.c
./setup_t -n minivmac -t mcar -m Plus -api cco -e bgc -sgn 0 > setup.sh
sh setup.sh
sh tests/run_disk_images.sh
```

The runner also supports Linux with a generated Linux configuration. It needs
Clang or GCC with ASan/UBSan. Keep `NonDiskProtect` and `Sony_SupportDC42`
enabled and `Sony_VerifyChecksums` disabled (the normal defaults). Tests also
work with `-sony-tag 1 -sony-sum 1`.

The test includes the production `SONYEMDV.c` detector and substitutes the host
disk callbacks with a synthetic header and file size. Unused emulation code is
removed by the linker; no ROM or guest disk is needed. Multi-gigabyte bounds
are exercised without allocating multi-gigabyte files.

Coverage includes raw HFS/MFS, valid partitions ending exactly at EOF, the last
scanned partition-map entry, valid partition offsets/sizes at 2 GiB and near
4 GiB, a later valid entry following an invalid one, empty and out-of-file
partitions, wrapping block values, and valid/truncated/wrapping Disk Copy 4.2
extents with and without tags. Mount rejection checks that the disk is ejected
and does not enter the mounted-drive mask.

The runner also checks that a zero-length parameter buffer can be created,
queried, and disposed. Empty clipboard text and the guest parameter-buffer
extension rely on that behavior.

Optional groups: `valid`, `partition-range`, `large-partition`, `dc42-range`.
For example, `sh tests/run_disk_images.sh large-partition` isolates the signed
arithmetic regression.
