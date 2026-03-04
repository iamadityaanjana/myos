#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

typedef struct {
    const char* name;
    uint32_t start;
    uint32_t end;
} memory_region_t;

void memory_init();
void* kmalloc(uint32_t size);
void* kmalloc_aligned(uint32_t size, uint32_t align);

uint32_t kheap_start();
uint32_t kheap_end();
uint32_t kheap_total();
uint32_t kheap_used();
uint32_t kheap_free();

uint32_t kernel_start_addr();
uint32_t kernel_end_addr();

int memory_get_map(memory_region_t* out_regions, int max_regions);

#endif