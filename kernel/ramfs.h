#ifndef RAMFS_H
#define RAMFS_H

#include "types.h"

typedef struct {
    const char* name;
    const uint8_t* data;
    uint32_t size;
} ramfs_entry_t;

void ramfs_init();
int ramfs_count();
int ramfs_get_entry(int index, ramfs_entry_t* out_entry);
int ramfs_get_file(const char* name, const uint8_t** out_data, uint32_t* out_size);

#endif
