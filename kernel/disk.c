#include "disk.h"

#include "io.h"

#define ATA_IO_BASE 0x1F0
#define ATA_REG_DATA      (ATA_IO_BASE + 0)
#define ATA_REG_SECCOUNT0 (ATA_IO_BASE + 2)
#define ATA_REG_LBA0      (ATA_IO_BASE + 3)
#define ATA_REG_LBA1      (ATA_IO_BASE + 4)
#define ATA_REG_LBA2      (ATA_IO_BASE + 5)
#define ATA_REG_HDDEVSEL  (ATA_IO_BASE + 6)
#define ATA_REG_COMMAND   (ATA_IO_BASE + 7)
#define ATA_REG_STATUS    (ATA_IO_BASE + 7)

#define ATA_CMD_READ_PIO  0x20
#define ATA_CMD_WRITE_PIO 0x30

#define ATA_SR_BSY 0x80
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

#define ATA_TIMEOUT 100000

static void ata_delay_400ns() {
    (void)inb(ATA_REG_STATUS);
    (void)inb(ATA_REG_STATUS);
    (void)inb(ATA_REG_STATUS);
    (void)inb(ATA_REG_STATUS);
}

static int ata_wait_ready() {
    uint32_t timeout = ATA_TIMEOUT;
    while (timeout--) {
        uint8_t status = inb(ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            return 0;
        }
        if ((status & ATA_SR_BSY) == 0 && (status & ATA_SR_DRQ)) {
            return 1;
        }
    }
    return 0;
}

static int ata_wait_not_busy() {
    uint32_t timeout = ATA_TIMEOUT;
    while (timeout--) {
        uint8_t status = inb(ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            return 0;
        }
        if ((status & ATA_SR_BSY) == 0) {
            return 1;
        }
    }
    return 0;
}

static void ata_select_lba28(uint32_t lba) {
    outb(ATA_REG_HDDEVSEL, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    ata_delay_400ns();

    outb(ATA_REG_SECCOUNT0, 1);
    outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
}

int disk_read_sector(uint32_t lba, uint8_t* out_buffer) {
    if (!out_buffer) {
        return 0;
    }

    if (!ata_wait_not_busy()) {
        return 0;
    }

    ata_select_lba28(lba);
    outb(ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    if (!ata_wait_ready()) {
        return 0;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t word = inw(ATA_REG_DATA);
        out_buffer[i * 2] = (uint8_t)(word & 0xFF);
        out_buffer[i * 2 + 1] = (uint8_t)((word >> 8) & 0xFF);
    }

    return 1;
}

int disk_write_sector(uint32_t lba, const uint8_t* in_buffer) {
    if (!in_buffer) {
        return 0;
    }

    if (!ata_wait_not_busy()) {
        return 0;
    }

    ata_select_lba28(lba);
    outb(ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    if (!ata_wait_ready()) {
        return 0;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t word = (uint16_t)in_buffer[i * 2] | ((uint16_t)in_buffer[i * 2 + 1] << 8);
        outw(ATA_REG_DATA, word);
    }

    ata_delay_400ns();
    return ata_wait_not_busy();
}
