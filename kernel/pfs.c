#include "pfs.h"

#include "disk.h"

#define PFS_MAGIC 0x31534650u
#define PFS_VERSION 1u

#define PFS_SUPERBLOCK_LBA 1u
#define PFS_DIR_START_LBA 2u
#define PFS_DIR_SECTORS 2u
#define PFS_DATA_START_LBA (PFS_DIR_START_LBA + PFS_DIR_SECTORS)

#define PFS_MAX_FILES 32
#define PFS_FILE_SLOT_SIZE 480u

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t max_files;
    uint32_t data_start_lba;
} pfs_superblock_t;

typedef struct {
    uint8_t used;
    char name[PFS_NAME_MAX];
    uint16_t size;
    uint32_t data_lba;
    uint8_t reserved[9];
} pfs_dir_entry_t;

static uint8_t g_ready = 0;

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

static void str_copy_name(char* dst, const char* src) {
    int i = 0;
    while (src[i] && i < (PFS_NAME_MAX - 1)) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int valid_name(const char* name) {
    if (!name || !name[0]) {
        return 0;
    }

    for (int i = 0; name[i]; i++) {
        char c = name[i];
        if (c == ' ' || c == '/' || c == '\\' || c == ':') {
            return 0;
        }
        if (i >= (PFS_NAME_MAX - 1)) {
            return 0;
        }
    }

    return 1;
}

static int read_directory(pfs_dir_entry_t* out_entries) {
    uint8_t* raw = (uint8_t*)out_entries;
    for (uint32_t i = 0; i < PFS_DIR_SECTORS; i++) {
        if (!disk_read_sector(PFS_DIR_START_LBA + i, raw + (i * 512u))) {
            return 0;
        }
    }
    return 1;
}

static int write_directory(const pfs_dir_entry_t* entries) {
    const uint8_t* raw = (const uint8_t*)entries;
    for (uint32_t i = 0; i < PFS_DIR_SECTORS; i++) {
        if (!disk_write_sector(PFS_DIR_START_LBA + i, raw + (i * 512u))) {
            return 0;
        }
    }
    return 1;
}

static void pfs_zero(uint8_t* buf, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = 0;
    }
}

static int pfs_format_new() {
    pfs_superblock_t sb;
    pfs_dir_entry_t dir[PFS_MAX_FILES];
    uint8_t sector[512];

    sb.magic = PFS_MAGIC;
    sb.version = PFS_VERSION;
    sb.max_files = PFS_MAX_FILES;
    sb.data_start_lba = PFS_DATA_START_LBA;

    pfs_zero((uint8_t*)dir, sizeof(dir));

    pfs_zero(sector, sizeof(sector));
    *((pfs_superblock_t*)sector) = sb;
    if (!disk_write_sector(PFS_SUPERBLOCK_LBA, sector)) {
        return 0;
    }

    if (!write_directory(dir)) {
        return 0;
    }

    pfs_zero(sector, sizeof(sector));
    for (uint32_t i = 0; i < PFS_MAX_FILES; i++) {
        if (!disk_write_sector(PFS_DATA_START_LBA + i, sector)) {
            return 0;
        }
    }

    return 1;
}

void pfs_init() {
    uint8_t sector[512];
    pfs_superblock_t* sb = (pfs_superblock_t*)sector;

    g_ready = 0;
    if (!disk_read_sector(PFS_SUPERBLOCK_LBA, sector)) {
        return;
    }

    if (sb->magic != PFS_MAGIC || sb->version != PFS_VERSION || sb->max_files != PFS_MAX_FILES) {
        if (!pfs_format_new()) {
            return;
        }
    }

    g_ready = 1;
}

int pfs_is_ready() {
    return g_ready ? 1 : 0;
}

int pfs_list(pfs_file_info_t* out_items, int max_items) {
    pfs_dir_entry_t dir[PFS_MAX_FILES];
    int written = 0;

    if (!g_ready || !out_items || max_items <= 0) {
        return 0;
    }

    if (!read_directory(dir)) {
        return 0;
    }

    for (int i = 0; i < PFS_MAX_FILES && written < max_items; i++) {
        if (!dir[i].used) {
            continue;
        }
        str_copy_name(out_items[written].name, dir[i].name);
        out_items[written].size = dir[i].size;
        written++;
    }

    return written;
}

static int find_file_index(pfs_dir_entry_t* dir, const char* name) {
    for (int i = 0; i < PFS_MAX_FILES; i++) {
        if (dir[i].used && str_equals(dir[i].name, name)) {
            return i;
        }
    }
    return -1;
}

static int find_free_index(pfs_dir_entry_t* dir) {
    for (int i = 0; i < PFS_MAX_FILES; i++) {
        if (!dir[i].used) {
            return i;
        }
    }
    return -1;
}

int pfs_read_file(const char* name, uint8_t* out_data, uint32_t out_capacity, uint32_t* out_size) {
    pfs_dir_entry_t dir[PFS_MAX_FILES];
    uint8_t sector[512];

    if (out_size) {
        *out_size = 0;
    }

    if (!g_ready || !name || !out_data || !out_size) {
        return 0;
    }

    if (!read_directory(dir)) {
        return 0;
    }

    int idx = find_file_index(dir, name);
    if (idx < 0) {
        return 0;
    }

    if (!disk_read_sector(dir[idx].data_lba, sector)) {
        return 0;
    }

    uint32_t to_copy = dir[idx].size;
    if (to_copy > out_capacity) {
        to_copy = out_capacity;
    }

    for (uint32_t i = 0; i < to_copy; i++) {
        out_data[i] = sector[i];
    }

    *out_size = to_copy;
    return 1;
}

int pfs_write_file(const char* name, const uint8_t* data, uint32_t size) {
    pfs_dir_entry_t dir[PFS_MAX_FILES];
    uint8_t sector[512];
    int idx;

    if (!g_ready || !valid_name(name) || !data || size > PFS_FILE_SLOT_SIZE) {
        return 0;
    }

    if (!read_directory(dir)) {
        return 0;
    }

    idx = find_file_index(dir, name);
    if (idx < 0) {
        idx = find_free_index(dir);
        if (idx < 0) {
            return 0;
        }
        dir[idx].used = 1;
        str_copy_name(dir[idx].name, name);
        dir[idx].data_lba = PFS_DATA_START_LBA + (uint32_t)idx;
    }

    pfs_zero(sector, sizeof(sector));
    for (uint32_t i = 0; i < size; i++) {
        sector[i] = data[i];
    }

    dir[idx].size = (uint16_t)size;

    if (!disk_write_sector(dir[idx].data_lba, sector)) {
        return 0;
    }

    if (!write_directory(dir)) {
        return 0;
    }

    return 1;
}

int pfs_delete_file(const char* name) {
    pfs_dir_entry_t dir[PFS_MAX_FILES];
    uint8_t sector[512];

    if (!g_ready || !valid_name(name)) {
        return 0;
    }

    if (!read_directory(dir)) {
        return 0;
    }

    int idx = find_file_index(dir, name);
    if (idx < 0) {
        return 0;
    }

    pfs_zero(sector, sizeof(sector));
    if (!disk_write_sector(dir[idx].data_lba, sector)) {
        return 0;
    }

    pfs_zero((uint8_t*)&dir[idx], sizeof(pfs_dir_entry_t));
    if (!write_directory(dir)) {
        return 0;
    }

    return 1;
}
