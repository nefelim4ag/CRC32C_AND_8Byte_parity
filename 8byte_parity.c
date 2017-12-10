#include <stdio.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

// Hashes
#include "xxhash.h"
#include "crc32.h"

static uint32_t parity_32(uint8_t *data, uint64_t byte_len, uint32_t seed) {
        uint32_t parity = 0;
        uint32_t *ptr = (uint32_t *) data;

        if (byte_len%sizeof(parity) != 0)
                printf("Data size must be aligned to: %lu\n", sizeof(parity));

        for (uint32_t i = 0; i < byte_len/sizeof(parity); i++)
                parity ^= ptr[i];

        return parity ^ seed;
}

/**
 * fparity32 - use several registres for computing data 32-bit parity
 * @data - input data stream
 * @byte_len - input data lengh in bytes
 * @seed - for altering parity value
 */

static uint32_t fparity32(const void *data, uint64_t byte_len, uint64_t seed) {
        uint32_t p1 = 0, p2 = 0, p3 = 0, p4 = 0;
        uint32_t *ptr = (uint32_t *) data;
        uint64_t index, index_end;
        uint32_t ret = 0;

        if (byte_len%sizeof(ret)) {
                printf("Data size must be aligned to: %lu\n", sizeof(ret));
                return -1;
        }

        index_end = byte_len/sizeof(ret);

        for (index = 0; index < index_end;) {
                switch ((index - index_end)%4) {
                        case 0:
                                p1 ^= ptr[index++];
                                p2 ^= ptr[index++];
                                p3 ^= ptr[index++];
                                p4 ^= ptr[index++];
                        break;
                        case 1:
                                p1 ^= ptr[index++];
                        break;
                        case 2:
                                p1 ^= ptr[index++];
                                p2 ^= ptr[index++];
                        break;
                        case 3:
                                p1 ^= ptr[index++];
                                p2 ^= ptr[index++];
                                p3 ^= ptr[index++];
                        break;
                }
        }

        ret = p1 ^ p2 ^ p3 ^ p4 ^ seed;

        return ret;
}

static uint64_t parity_64(const void *data, uint64_t byte_len, uint64_t seed) {
        uint64_t parity = 0;
        uint64_t *ptr = (uint64_t *) data;

        if (byte_len%sizeof(parity) != 0)
                printf("Data size must be aligned to: %lu\n", sizeof(parity));

        for (uint64_t i = 0; i < byte_len/sizeof(parity); i++)
                parity ^= ptr[i];

        return parity ^ seed;
}

/**
 * fparity64 - use several registres for computing data 64-bit parity
 * @data - input data stream
 * @byte_len - input data lengh in bytes
 * @seed - for altering parity value
 */

static uint64_t fparity64(const void *data, uint64_t byte_len, uint64_t seed) {
        uint64_t p1 = 0, p2 = 0, p3 = 0, p4 = 0;
        uint64_t *ptr = (uint64_t *) data;
        uint64_t index, index_end;
        uint64_t ret = 0;

        if (byte_len%sizeof(ret)) {
                printf("Data size must be aligned to: %lu\n", sizeof(ret));
                return -1;
        }

        index_end = byte_len/sizeof(ret);

        for (index = 0; index < index_end;) {
                switch ((index - index_end)%4) {
                        case 0:
                                p1 ^= ptr[index++];
                                p2 ^= ptr[index++];
                                p3 ^= ptr[index++];
                                p4 ^= ptr[index++];
                        break;
                        case 1:
                                p1 ^= ptr[index++];
                        break;
                        case 2:
                                p1 ^= ptr[index++];
                                p2 ^= ptr[index++];
                        break;
                        case 3:
                                p1 ^= ptr[index++];
                                p2 ^= ptr[index++];
                                p3 ^= ptr[index++];
                        break;
                }
        }

        ret = p1 ^ p2 ^ p3 ^ p4 ^ seed;

        return ret;
}

#define PAGE_SIZE (4*1024)

int main() {
        uint8_t PAGE[PAGE_SIZE];
        uint64_t i;
        uint64_t iter = 1024;
        clock_t start, end;
        srand(time(NULL));


        iter *= 1024*1024*4/PAGE_SIZE;
        iter += rand()%4096;

        for (uint32_t i = 0; i < PAGE_SIZE; i++)
                PAGE[i] = rand()%255;

        printf("--- Test speed of hash/parity functions ---\n");

        printf("PAGE_SIZE: %u, loop count: %lu\n", PAGE_SIZE, iter);

        /*
        {
                uint64_t parity64;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity64 = parity_64((uint8_t *) &PAGE, PAGE_SIZE, 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("Parity64:\t0x%" PRIx64 "\t\t\tperf: %lu µs,\tth: %f MiB/s\n", parity64, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }
        */

        {
                uint64_t parity64;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity64 = fparity64((uint8_t *) &PAGE, PAGE_SIZE, 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("FParity64:\t0x%" PRIx64 "\t\t\tperf: %lu µs,\tth: %f MiB/s\n", parity64, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                uint32_t parity32;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity32 = parity_32((uint8_t *) &PAGE, PAGE_SIZE, 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("parity32:\t0x%" PRIx32 "\t\t\t\tperf: %lu µs,\tth: %f MiB/s\n", parity32, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                uint32_t parity32;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity32 = fparity32((uint8_t *) &PAGE, PAGE_SIZE, 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("fparity32:\t0x%" PRIx32 "\t\t\t\tperf: %lu µs,\tth: %f MiB/s\n", parity32, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                uint32_t crc;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { crc = crc32c(0, &PAGE, PAGE_SIZE); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("crc32hw:\t0x%" PRIx32 "\t\t\t\tperf: %lu µs,\tth: %f MiB/s\n", crc, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                uint64_t hash64;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { hash64 = xxh64(&PAGE, PAGE_SIZE, 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("xxhash64:\t0x%" PRIx64 "\t\t\tperf: %lu µs,\tth: %f MiB/s\n", hash64, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        /* Try add error and fix it */
        printf("--- Example of error injection and fixup ---\n");

        {
                uint64_t orig_seed = 0xbadc3cc0de;
                uint64_t orig_parity = parity_64((uint8_t *) &PAGE, PAGE_SIZE, orig_seed);
                uint32_t orig_crc = crc32c(0, &PAGE, PAGE_SIZE);
                uint64_t orig_xxhash64 = xxh64(&PAGE, PAGE_SIZE, orig_seed);
                uint64_t stripe_num = PAGE_SIZE/sizeof(orig_parity);
                uint64_t *ptr = (uint64_t *) &PAGE;

                printf("Stripe num: %lu\n", stripe_num);

                PAGE[rand()%PAGE_SIZE] = rand()%256;

                /* Brute force broken part */
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < stripe_num; i++) {
                        uint64_t stripe_backup = ptr[i];
                        ptr[i] = orig_parity;
                        ptr[i] = fparity64((uint8_t *) ptr, PAGE_SIZE, orig_seed);
                        uint32_t current_crc = crc32c(0, ptr, PAGE_SIZE);
                        if (orig_crc == current_crc) {
                                break;
                        } else {
                                ptr[i] = stripe_backup;
                        }

                }
                end = clock()*1000000/CLOCKS_PER_SEC;

                printf("ERR OFFSET: 0x%" PRIx64 "| Block CRC32c: 0x%" PRIx32 " - probably fixed\n", i*sizeof(orig_parity), orig_crc);

                printf("perf: %lu µs,\tth: %f MiB/s\n", (end - start), PAGE_SIZE*i*1.0/(end - start));

                uint64_t new_xxhash64 = xxh64(ptr, PAGE_SIZE, orig_seed);
                if (new_xxhash64 == orig_xxhash64) {
                        printf("xxhash64: match\n");
                } else {
                        printf("xxhash64: not match\n");
                }
        }

        /* Try add error and fix it */
        printf("--- Example of stupid fix on 1 bit flip injection and fixup by CRC32C ---\n");

        {
                uint64_t orig_seed = 0xbadc3cc0de;
                uint32_t orig_crc = crc32c(0, &PAGE, PAGE_SIZE);
                uint64_t orig_xxhash64 = xxh64(&PAGE, PAGE_SIZE, orig_seed);
                uint32_t rand_offset = rand()%PAGE_SIZE;
                uint32_t current_crc;
                int search = 1;

                printf("Old byte: 0x%" PRIx32 "\n", PAGE[rand_offset]);
                PAGE[rand_offset] |= 0x2;
                printf("New byte: 0x%" PRIx32 "\n", PAGE[rand_offset]);

                /* Brute force broken part */
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < PAGE_SIZE && search; i++) {
                        for (int a = 7; a >= 0; a--) {
                                PAGE[i] ^= 0x1 << a;
                                current_crc = crc32c(0, &PAGE, PAGE_SIZE);
                                if (orig_crc == current_crc) {
                                        printf("Fixed! sic!!!\n");
                                        search = 0;
                                        break;
                                }
                                PAGE[i] ^= 0x1 << a;
                        }
                }
                end = clock()*1000000/CLOCKS_PER_SEC;

                if (!search)
                        printf("ERR OFFSET: 0x%" PRIx64 "| Block CRC32c: 0x%" PRIx32 " - probably fixed\n", i, orig_crc);

                printf("perf: %lu µs,\tth: %f MiB/s\n", (end - start), PAGE_SIZE*i*1.0/(end - start));

                uint64_t new_xxhash64 = xxh64(&PAGE, PAGE_SIZE, orig_seed);
                if (new_xxhash64 == orig_xxhash64) {
                        printf("xxhash64: match\n");
                } else {
                        printf("xxhash64: not match\n");
                }
        }

        /* Try add error and fix it */
        printf("--- Example of stupid fix on 2 bit flip injection and fixup by CRC32C ---\n");

        {
                uint64_t orig_seed = 0xbadc3cc0de;
                uint32_t orig_crc = crc32c(0, &PAGE, PAGE_SIZE);
                uint64_t orig_xxhash64 = xxh64(&PAGE, PAGE_SIZE, orig_seed);
                uint32_t rand_offset = rand()%PAGE_SIZE;
                uint32_t current_crc;
                int search = 1;

                printf("Old byte: 0x%" PRIx32 "\n", PAGE[rand_offset]);
                PAGE[rand_offset] &= 252;
                printf("New byte: 0x%" PRIx32 "\n", PAGE[rand_offset]);

                /* Brute force broken part */
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < PAGE_SIZE && search; i++) {
                        for (int a = 7; a; a--) {
                                for (int b = a; b; b--) {
                                        PAGE[i] ^= 0x1 << a | (0x1 << b);
                                        current_crc = crc32c(0, &PAGE, PAGE_SIZE);
                                        if (orig_crc == current_crc) {
                                                printf("Fixed! sic!!!\n");
                                                search = 0;
                                                break;
                                        }
                                        PAGE[i] ^= 0x1 << a | (0x1 << b);
                                }
                        }
                }
                end = clock()*1000000/CLOCKS_PER_SEC;

                if (!search)
                        printf("ERR OFFSET: 0x%" PRIx64 "| Block CRC32c: 0x%" PRIx32 " - probably fixed\n", i, orig_crc);

                printf("perf: %lu µs,\tth: %f MiB/s\n", (end - start), PAGE_SIZE*i*1.0/(end - start));

                uint64_t new_xxhash64 = xxh64(&PAGE, PAGE_SIZE, orig_seed);
                if (new_xxhash64 == orig_xxhash64) {
                        printf("xxhash64: match\n");
                } else {
                        printf("xxhash64: not match\n");
                }
        }

        return 0;
}
