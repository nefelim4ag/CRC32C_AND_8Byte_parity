#include <stdio.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

// Hashes
#include "xxhash.h"
#include "crc32.h"

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

#define ALIGN(x, y) ((x - x % y)/y)

static inline uint8_t bitshift(int shift) {
        static const uint8_t jump_table[8] = {
                1 << 0, 1 << 1, 1 << 2, 1 << 3,
                1 << 4, 1 << 5, 1 << 6, 1 << 7
        };
        return jump_table[shift % 8];
}

struct crc32_correction {
        /* Memory with crc32 missmatch */
        void *memory;
        size_t size;
        /* CRC32 to match */
        unsigned orig_crc;
        /* Byte offset with fixed error */
        size_t error_offset;
        int fixed:1;
};

static void crc32_bitflip_corrector(struct crc32_correction *data) {
        const size_t size = data->size;
        const size_t bits_to_flip = data->size * 8;

        char *ptr = (char *)data->memory;
        unsigned i, j = 0;

        /* Brute force 1-bit error */
        for (i = 0; i < bits_to_flip; i++) {
                ptr[ALIGN(i, 8)] ^= bitshift(i);
                if (data->orig_crc == crc32c(0, data->memory, size))
                        goto out;
                ptr[ALIGN(i, 8)] ^= bitshift(i);
        }

        printf("lol");

        /* Brute force err 2-bits error */
        for (i = 0; i < bits_to_flip; i++) {
                ptr[ALIGN(i, 8)] ^= bitshift(i);
                for (j = i + 1; j < bits_to_flip; j++) {
                        ptr[ALIGN(j, 8)] ^= bitshift(j);
                        if (data->orig_crc == crc32c(0, data->memory, size))
                                goto out;
                        ptr[ALIGN(j, 8)] ^= bitshift(j);
                }
                ptr[ALIGN(i, 8)] ^= bitshift(i);
        }

        return;

        out:
                printf("ERR OFFSET: 0x%" PRIx32 " 0x%" PRIx32 "\n", ALIGN(i, 8), ALIGN(j, 8));
                data->error_offset = ALIGN(i, 8);
                data->fixed = 1;
}

static void corrupt_random_bit(void *ptr, size_t size) {
        uint8_t *memory = (uint8_t *) ptr;
        uint32_t rand_offset = rand() % size;
        uint8_t old, new;
        old = memory[rand_offset];
        while (memory[rand_offset] == old) {
                memory[rand_offset] |= 1 << rand() % 7;
        }
        new = memory[rand_offset];
        printf("Byte at 0x%" PRIx32  ": 0x%" PRIx32 " -> 0x%" PRIx32 "\n", rand_offset, old, new);
}

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
                uint32_t parity32;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity32 = fparity32((uint8_t *) &PAGE, PAGE_SIZE, i); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("fparity32:\t0x%" PRIx32 "\t\t\t\tperf: %lu µs,\tth: %.2f MiB/s\n", parity32, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                volatile uint64_t parity64;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { parity64 = fparity64((uint8_t *) &PAGE, PAGE_SIZE, i); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("fparity64:\t0x%" PRIx64 "\t\t\tperf: %lu µs,\tth: %.2f MiB/s\n", parity64, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                uint32_t crc;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { crc = crc32c(0, &PAGE, PAGE_SIZE); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("crc32hw:\t0x%" PRIx32 "\t\t\t\tperf: %lu µs,\tth: %.2f MiB/s\n", crc, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        {
                uint64_t hash64;
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < iter; i++) { hash64 = xxh64(&PAGE, PAGE_SIZE, 0); }
                end = clock()*1000000/CLOCKS_PER_SEC;
                printf("xxhash64:\t0x%" PRIx64 "\t\t\tperf: %lu µs,\tth: %.2f MiB/s\n", hash64, (end - start), PAGE_SIZE*iter*1.0/(end - start));
        }

        /* Try add error and fix it */
        printf("--- Example of error injection and fixup ---\n");

        {
                uint64_t orig_parity = fparity64((uint8_t *) &PAGE, PAGE_SIZE, 0);
                uint64_t orig_xxhash64 = xxh64(&PAGE, PAGE_SIZE, 0);
                uint64_t stripe_num = PAGE_SIZE/sizeof(orig_parity);
                uint64_t *ptr = (uint64_t *) &PAGE;

                printf("Stripe num: %lu by %lu byte\n", stripe_num, sizeof(orig_parity));

                corrupt_random_bit(&PAGE, PAGE_SIZE);

                /* Brute force broken part */
                start = clock()*1000000/CLOCKS_PER_SEC;
                for (i = 0; i < stripe_num; i++) {
                        uint64_t stripe_backup = ptr[i];
                        ptr[i] = orig_parity;
                        ptr[i] = fparity64((uint8_t *) ptr, PAGE_SIZE, 0);
                        uint64_t new_xxhash64 = xxh64(ptr, PAGE_SIZE, 0);
                        if (new_xxhash64 == orig_xxhash64) {
                                break;
                        } else {
                                ptr[i] = stripe_backup;
                        }

                }
                end = clock()*1000000/CLOCKS_PER_SEC;

                printf("ERR OFFSET: 0x%" PRIx64 " probably fixed\n", i*sizeof(orig_parity));

                printf("perf: %lu µs,\tth: %f MiB/s\n", (end - start), PAGE_SIZE*i*1.0/(end - start));

                uint64_t new_xxhash64 = xxh64(ptr, PAGE_SIZE, 0);
                if (new_xxhash64 == orig_xxhash64) {
                        printf("xxhash64: match\n");
                } else {
                        printf("xxhash64: not match\n");
                }
        }

        /* Try add error and fix it */
        printf("--- Example of stupid fix on 1 bit flip injection and fixup by CRC32C ---\n");

        {
                static struct crc32_correction info;

                uint32_t orig_crc = crc32c(0, &PAGE, PAGE_SIZE);
                uint64_t orig_xxhash64 = xxh64(&PAGE, PAGE_SIZE, 0);

                corrupt_random_bit(&PAGE, PAGE_SIZE);

                info.memory = &PAGE;
                info.size = PAGE_SIZE;
                info.orig_crc = orig_crc;
                info.error_offset = 0;
                info.fixed = 0;


                start = clock()*1000000/CLOCKS_PER_SEC;
                crc32_bitflip_corrector(&info);
                end = clock()*1000000/CLOCKS_PER_SEC;

                if (info.fixed)
                        printf("ERR OFFSET: 0x%" PRIx64 "| Block CRC32c: 0x%" PRIx32 " - probably fixed\n", info.error_offset, orig_crc);

                printf("perf: %lu µs,\tth: %f MiB/s\n", (end - start), info.error_offset * i * 1.0 / (end - start));

                if (orig_xxhash64 == xxh64(&PAGE, PAGE_SIZE, 0)) {
                        printf("xxhash64: match\n");
                } else {
                        printf("xxhash64: not match\n");
                }
        }

        /* Try add error and fix it */
        printf("--- Example of stupid fix on 2 bit flip injection and fixup by CRC32C ---\n");

        {
                static struct crc32_correction info;
                uint64_t orig_xxhash64 = xxh64(&PAGE, PAGE_SIZE, 0);

                info.memory = &PAGE;
                info.size = PAGE_SIZE;
                info.orig_crc = crc32c(0, &PAGE, PAGE_SIZE);
                info.error_offset = 0;
                info.fixed = 0;

                corrupt_random_bit(&PAGE, PAGE_SIZE);
                corrupt_random_bit(&PAGE, PAGE_SIZE);

                start = clock()*1000000/CLOCKS_PER_SEC;
                crc32_bitflip_corrector(&info);
                end = clock()*1000000/CLOCKS_PER_SEC;

                if (info.fixed)
                        printf("ERR OFFSET: 0x%" PRIx64 "| Block CRC32c: 0x%" PRIx32 " - probably fixed\n", i, info.orig_crc);

                printf("perf: %lu µs,\tth: %f MiB/s\n", (end - start), PAGE_SIZE*i*1.0/(end - start));
                printf("perf: %lu ms,\tth: %f MiB/s\n", (end - start)/1000, PAGE_SIZE*i*1.0/(end - start));
                printf("perf: %lu s,\tth: %f MiB/s\n", (end - start)/1000/1000, PAGE_SIZE*i*1.0/(end - start));
                if (orig_xxhash64 == xxh64(&PAGE, PAGE_SIZE, 0)) {
                        printf("xxhash64: match\n");
                } else {
                        printf("xxhash64: not match\n");
                }
        }

        return 0;
}
