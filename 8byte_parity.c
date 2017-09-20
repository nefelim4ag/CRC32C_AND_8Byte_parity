#include <stdio.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

// Hashes
#include "xxhash.h"
#include "crc32.h"

static uint32_t parity_32(uint8_t *data, uint64_t byte_len, uint32_t seed) {
        uint32_t parity = seed;
        uint32_t step = sizeof(parity);
        uint32_t *ptr = (uint32_t *) data;

        for (uint32_t i = 0; i < byte_len/step; i++) {
                parity ^= *ptr;
                ptr++;
        }

        return parity;
}

static uint64_t parity_64(const void *data, uint64_t byte_len, uint64_t seed) {
        uint64_t parity = seed;
        uint64_t *ptr = (uint64_t *) data;

        if (byte_len%sizeof(parity) != 0)
                printf("Not supported!\n");

        for (uint64_t i = 0; i < byte_len/sizeof(parity); i++) {
                parity ^= *ptr;
                ptr++;
        }

        return parity;
}

//int parity_fix_error

int main() {
        uint8_t PAGE[4*1024];
        uint64_t i;
        uint64_t iter = 1024;
        clock_t start, end;
        srand(time(NULL));


        iter *= 1024*1024*4/sizeof(PAGE);

        for (uint32_t i = 0; i < sizeof(PAGE); i++)
                PAGE[i] = rand()%256;

        printf("--- Test speed of hash/parity functions ---\n");

        printf("PAGE_SIZE: %lu, loop count: %lu\n", sizeof(PAGE), iter);

        {
                uint64_t parity64;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity64 = parity_64((uint8_t *) &PAGE, sizeof(PAGE), 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("Parity64:\t0x%" PRIx64 "\t\t\tperf: %lu µs,\tth: %f MiB/s\n", parity64, (end - start), sizeof(PAGE)*iter*1.0/(end - start));
        }

        {
                uint32_t parity32;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity32 = parity_32((uint8_t *) &PAGE, sizeof(PAGE), 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("parity32:\t0x%" PRIx32 "\t\t\t\tperf: %lu µs,\tth: %f MiB/s\n", parity32, (end - start), sizeof(PAGE)*iter*1.0/(end - start));
        }

        {
                uint32_t crc;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { crc = crc32c(0, &PAGE, sizeof(PAGE)); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("crc32:\t\t0x%" PRIx32 "\t\t\t\tperf: %lu µs,\tth: %f MiB/s\n", crc, (end - start), sizeof(PAGE)*iter*1.0/(end - start));
        }

        {
                uint64_t hash64;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { hash64 = xxh64(&PAGE, sizeof(PAGE), 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("xxhash64:\t0x%" PRIx64 "\t\t\tperf: %lu µs,\tth: %f MiB/s\n", hash64, (end - start), sizeof(PAGE)*iter*1.0/(end - start));
        }

        /* Try add error and fix it */
        printf("--- Example of error injection and fixup ---\n");

        {
                uint64_t orig_seed = 0xbadc3cc0de;
                uint64_t orig_parity = parity_64((uint8_t *) &PAGE, sizeof(PAGE), orig_seed);
                uint64_t orig_xxhash64 = xxh64(&PAGE, sizeof(PAGE), orig_seed);
                uint32_t orig_crc = crc32c(0, &PAGE, sizeof(PAGE));
                uint64_t stripe_num = sizeof(PAGE)/sizeof(orig_parity);
                uint64_t PAGE_FIXED[stripe_num];

                memcpy(&PAGE_FIXED, &PAGE, sizeof(PAGE));

                printf("Stripe num: %lu\n", stripe_num);

                PAGE[rand()%sizeof(PAGE)] = rand()%256;

                /* Brute force broken part */
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < stripe_num; i++) {
                        memcpy(&PAGE_FIXED, &PAGE, sizeof(PAGE));
                        PAGE_FIXED[i] = orig_parity;
                        PAGE_FIXED[i] = parity_64((uint8_t *) &PAGE_FIXED, sizeof(PAGE_FIXED), orig_seed);

                        if (orig_crc == crc32c(0, &PAGE_FIXED, sizeof(PAGE_FIXED))) {
                                printf("Error found at: 0x%" PRIx64 "\n", i*sizeof(orig_parity));
                                printf("CRC32C: Match!\n");
                                printf("Probably fixed...\n");
                                break;
                        }
                }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("perf: %lu µs,\tth: %f MiB/s\n", (end - start), sizeof(PAGE)*i*1.0/(end - start));

                uint64_t new_xxhash64 = xxh64(&PAGE_FIXED, sizeof(PAGE_FIXED), orig_seed);
                if (new_xxhash64 == orig_xxhash64) {
                        printf("xxhash64: matched!\n");
                        printf("Error was fixed!\n");
                }

        }

        return 0;
}
