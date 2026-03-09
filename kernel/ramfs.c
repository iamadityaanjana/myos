#include "ramfs.h"

static const uint8_t file_readme[] =
    "MyOS RAMFS\n"
    "----------\n"
    "This is a tiny read-only in-kernel filesystem.\n"
    "Commands:\n"
    "  ls\n"
    "  cat <file>\n"
    "  hexdump <file>\n";

static const uint8_t file_motd[] =
    "Welcome to MyOS.\n"
    "Build boldly, debug patiently.\n";

static const uint8_t file_kernel_cfg[] =
    "timer_hz=100\n"
    "scheduler=round_robin\n"
    "filesystem=ramfs_ro\n";

static const uint8_t file_logo_bin[] = {
    0x4D, 0x59, 0x4F, 0x53, 0x00, 0x01, 0x02, 0x03,
    0x7F, 0x45, 0x4C, 0x46, 0xAA, 0x55, 0x10, 0x20,
    0x30, 0x40, 0x50, 0x60
};

static ramfs_entry_t g_files[] = {
    {"README.TXT", file_readme, (uint32_t)(sizeof(file_readme) - 1)},
    {"MOTD.TXT", file_motd, (uint32_t)(sizeof(file_motd) - 1)},
    {"KERNEL.CFG", file_kernel_cfg, (uint32_t)(sizeof(file_kernel_cfg) - 1)},
    {"LOGO.BIN", file_logo_bin, (uint32_t)(sizeof(file_logo_bin))}
};

static int str_equals(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return a[i] == b[i];
}

void ramfs_init() {
    // Static in-kernel image; no runtime mount needed yet.
}

int ramfs_count() {
    return (int)(sizeof(g_files) / sizeof(g_files[0]));
}

int ramfs_get_entry(int index, ramfs_entry_t* out_entry) {
    int count = ramfs_count();
    if (!out_entry || index < 0 || index >= count) {
        return 0;
    }

    out_entry->name = g_files[index].name;
    out_entry->data = g_files[index].data;
    out_entry->size = g_files[index].size;
    return 1;
}

int ramfs_get_file(const char* name, const uint8_t** out_data, uint32_t* out_size) {
    if (!name || !out_data || !out_size) {
        return 0;
    }

    int count = ramfs_count();
    for (int i = 0; i < count; i++) {
        if (str_equals(name, g_files[i].name)) {
            *out_data = g_files[i].data;
            *out_size = g_files[i].size;
            return 1;
        }
    }

    return 0;
}
