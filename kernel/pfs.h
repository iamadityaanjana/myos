#ifndef PFS_H
#define PFS_H

#include "types.h"

#define PFS_NAME_MAX 16

typedef struct {
    char name[PFS_NAME_MAX];
    uint16_t size;
} pfs_file_info_t;

void pfs_init();
int pfs_is_ready();
int pfs_list(pfs_file_info_t* out_items, int max_items);
int pfs_read_file(const char* name, uint8_t* out_data, uint32_t out_capacity, uint32_t* out_size);
int pfs_write_file(const char* name, const uint8_t* data, uint32_t size);
int pfs_delete_file(const char* name);

#endif
