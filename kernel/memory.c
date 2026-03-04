#include "memory.h"

#define KERNEL_HEAP_SIZE (128 * 1024)

extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

static uint32_t heap_start_addr = 0;
static uint32_t heap_end_addr = 0;
static uint32_t heap_curr_addr = 0;

static uint32_t align_up(uint32_t value, uint32_t align) {
    return (value + (align - 1)) & ~(align - 1);
}

void memory_init() {
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    heap_start_addr = align_up(kernel_end, 16);
    heap_end_addr = heap_start_addr + KERNEL_HEAP_SIZE;
    heap_curr_addr = heap_start_addr;
}

void* kmalloc(uint32_t size) {
    if (size == 0) {
        return 0;
    }

    size = align_up(size, 8);

    if (heap_curr_addr + size > heap_end_addr) {
        return 0;
    }

    void* result = (void*)heap_curr_addr;
    heap_curr_addr += size;
    return result;
}

uint32_t kheap_start() {
    return heap_start_addr;
}

uint32_t kheap_end() {
    return heap_end_addr;
}

uint32_t kheap_total() {
    return heap_end_addr - heap_start_addr;
}

uint32_t kheap_used() {
    return heap_curr_addr - heap_start_addr;
}

uint32_t kheap_free() {
    return heap_end_addr - heap_curr_addr;
}

uint32_t kernel_start_addr() {
    return (uint32_t)&_kernel_start;
}

uint32_t kernel_end_addr() {
    return (uint32_t)&_kernel_end;
}

int memory_get_map(memory_region_t* out_regions, int max_regions) {
    if (!out_regions || max_regions <= 0) {
        return 0;
    }

    int i = 0;

    if (i < max_regions) {
        out_regions[i].name = "IVT+BDA";
        out_regions[i].start = 0x00000000;
        out_regions[i].end = 0x00000500;
        i++;
    }

    if (i < max_regions) {
        out_regions[i].name = "Bootloader";
        out_regions[i].start = 0x00007C00;
        out_regions[i].end = 0x00007E00;
        i++;
    }

    if (i < max_regions) {
        out_regions[i].name = "Kernel image";
        out_regions[i].start = kernel_start_addr();
        out_regions[i].end = kernel_end_addr();
        i++;
    }

    if (i < max_regions) {
        out_regions[i].name = "Kernel heap";
        out_regions[i].start = heap_start_addr;
        out_regions[i].end = heap_end_addr;
        i++;
    }

    if (i < max_regions) {
        out_regions[i].name = "Kernel stack";
        out_regions[i].start = 0x0008F000;
        out_regions[i].end = 0x00090000;
        i++;
    }

    if (i < max_regions) {
        out_regions[i].name = "VGA text";
        out_regions[i].start = 0x000B8000;
        out_regions[i].end = 0x000B9000;
        i++;
    }

    return i;
}