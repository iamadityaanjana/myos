#ifndef DISK_H
#define DISK_H

#include "types.h"

int disk_read_sector(uint32_t lba, uint8_t* out_buffer);
int disk_write_sector(uint32_t lba, const uint8_t* in_buffer);

#endif
