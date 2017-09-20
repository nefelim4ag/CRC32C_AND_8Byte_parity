# CRC32C_AND_8Byte_parity
Proof of concept 8Byte Parity + CRC32C as checksum

```
~$ make clean; make; ./8byte_parity
rm -fv *.o
removed '8byte_parity.o'
removed 'crc32.o'
removed 'xxhash.o'
cc -O2 -Wall -Werror -pthread -c 8byte_parity.c -o 8byte_parity.o
cc -O2 -Wall -Werror -pthread -c xxhash.c -o xxhash.o
cc -O2 -Wall -Werror -pthread -c crc32.c -o crc32.o
cc -O2 -Wall -Werror -pthread -o 8byte_parity 8byte_parity.o xxhash.o crc32.o
--- Test speed of hash/parity functions ---
PAGE_SIZE: 4096, loop count: 1048576
Parity64:       0x73844fb949a04d18                      perf: 241981 µs,        th: 17749.192275 MiB/s
parity32:       0x3a2402a1                              perf: 453929 µs,        th: 9461.760090 MiB/s
crc32:          0xcc5a1265                              perf: 311659 µs,        th: 13780.982728 MiB/s
xxhash64:       0x76d03edf67d0d854                      perf: 360420 µs,        th: 11916.562055 MiB/s
--- Example of error injection and fixup ---
Stripe num: 512
Error found at: 0xb28
CRC32C: Match!
Probably fixed...
perf: 210 µs,   th: 6963.200000 MiB/s
xxhash64: matched!
Error was fixed!
```
