#include <inttypes.h>

uint32_t crc32c(uint32_t crc, const void *buf, uint64_t len);
uint32_t crc32c_sw(uint32_t crci, const void *buf, uint64_t len);
