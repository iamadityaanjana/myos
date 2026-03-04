#include "paging.h"
#include "memory.h"

static uint32_t* page_directory = 0;
static uint32_t* first_page_table = 0;
static int paging_enabled = 0;

void paging_init() {
    page_directory = (uint32_t*)kmalloc_aligned(1024 * sizeof(uint32_t), 4096);
    first_page_table = (uint32_t*)kmalloc_aligned(1024 * sizeof(uint32_t), 4096);

    if (!page_directory || !first_page_table) {
        paging_enabled = 0;
        return;
    }

    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }

    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 0x3;
    }

    page_directory[0] = ((uint32_t)first_page_table) | 0x3;

    __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory));

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    paging_enabled = 1;
}

int paging_is_enabled() {
    return paging_enabled;
}