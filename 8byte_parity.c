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
        uint32_t *ptr = (uint32_t *) data;

        if (byte_len%sizeof(parity) != 0)
                printf("Not supported!\n");

        for (uint32_t i = 0; i < byte_len/sizeof(parity); i++)
                parity ^= ptr[i];

        return parity;
}

static uint64_t parity_64(const void *data, uint64_t byte_len, uint64_t seed) {
        uint64_t parity = seed;
        uint64_t *ptr = (uint64_t *) data;

        if (byte_len%sizeof(parity) != 0)
                printf("Not supported!\n");

        for (uint64_t i = 0; i < byte_len/sizeof(parity); i++)
                parity ^= ptr[i];

        return parity;
}

struct parity128 {
        uint64_t parity[2];
};

static struct parity128 parity_128(const void *data, uint64_t byte_len, struct parity128 seed) {
        struct parity128 parity;
        uint64_t *ptr = (uint64_t *) data;
        parity.parity[0] = seed.parity[0];
        parity.parity[1] = seed.parity[1];

        if (byte_len%sizeof(parity) != 0)
                printf("Not supported!\n");

        for (uint64_t i = 0; i < byte_len/sizeof(ptr); i+=2) {
                parity.parity[0] = ptr[i];
                parity.parity[1] = ptr[i+1];
        }

        return parity;
}

//int parity_fix_error

#define PAGE_SIZE (4*1024)

int main() {
        uint8_t PAGE[PAGE_SIZE];
        uint64_t i;
        uint64_t iter = 1024;
        clock_t start, end;
        srand(time(NULL));


        iter *= 1024*1024*4/PAGE_SIZE;

        for (uint32_t i = 0; i < PAGE_SIZE; i++)
                PAGE[i] = rand()%256;

        printf("--- Test speed of hash/parity functions ---\n");

        printf("PAGE_SIZE: %u, loop count: %lu\n", PAGE_SIZE, iter);

        {
                struct parity128 parity;
                parity.parity[0] = 0;
                parity.parity[1] = 0;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity = parity_128((uint8_t *) &PAGE, PAGE_SIZE, parity); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("Parity128:\t0x%" PRIx64 "%" PRIx64 "\tperf: %lu µs,\tth: %f MiB/s\n", parity.parity[0], parity.parity[1], (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                uint64_t parity64;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity64 = parity_64((uint8_t *) &PAGE, PAGE_SIZE, 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("Parity64:\t0x%" PRIx64 "\t\t\tperf: %lu µs,\tth: %f MiB/s\n", parity64, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                uint32_t parity32;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity32 = parity_32((uint8_t *) &PAGE, PAGE_SIZE, 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("parity32:\t0x%" PRIx32 "\t\t\t\tperf: %lu µs,\tth: %f MiB/s\n", parity32, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                uint32_t crc;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { crc = crc32c(0, &PAGE, PAGE_SIZE); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("crc32:\t\t0x%" PRIx32 "\t\t\t\tperf: %lu µs,\tth: %f MiB/s\n", crc, (end - start), PAGE_SIZE*iter*1.0/(end - start));
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
                        ptr[i] = parity_64((uint8_t *) ptr, PAGE_SIZE, orig_seed);
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

        return 0;
}
