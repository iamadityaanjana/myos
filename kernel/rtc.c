#include "rtc.h"

#include "io.h"

static uint8_t cmos_read(uint8_t reg) {
    outb(0x70, (uint8_t)(reg | 0x80));
    return inb(0x71);
}

static int rtc_is_updating() {
    return (cmos_read(0x0A) & 0x80) != 0;
}

static uint8_t bcd_to_bin(uint8_t value) {
    return (uint8_t)((value & 0x0F) + ((value >> 4) * 10));
}

void rtc_read_datetime(rtc_datetime_t* out) {
    if (!out) {
        return;
    }

    rtc_datetime_t first;
    rtc_datetime_t second;

    do {
        while (rtc_is_updating()) {
        }

        first.second = cmos_read(0x00);
        first.minute = cmos_read(0x02);
        first.hour = cmos_read(0x04);
        first.day = cmos_read(0x07);
        first.month = cmos_read(0x08);
        first.year = cmos_read(0x09);

        while (rtc_is_updating()) {
        }

        second.second = cmos_read(0x00);
        second.minute = cmos_read(0x02);
        second.hour = cmos_read(0x04);
        second.day = cmos_read(0x07);
        second.month = cmos_read(0x08);
        second.year = cmos_read(0x09);
    } while (first.second != second.second ||
             first.minute != second.minute ||
             first.hour != second.hour ||
             first.day != second.day ||
             first.month != second.month ||
             first.year != second.year);

    uint8_t reg_b = cmos_read(0x0B);
    int is_binary = (reg_b & 0x04) != 0;
    int is_24_hour = (reg_b & 0x02) != 0;

    uint8_t second_value = second.second;
    uint8_t minute_value = second.minute;
    uint8_t hour_value = second.hour;
    uint8_t day_value = second.day;
    uint8_t month_value = second.month;
    uint8_t year_value = (uint8_t)second.year;

    if (!is_binary) {
        second_value = bcd_to_bin(second_value);
        minute_value = bcd_to_bin(minute_value);
        day_value = bcd_to_bin(day_value);
        month_value = bcd_to_bin(month_value);
        year_value = bcd_to_bin(year_value);

        uint8_t hour_low = bcd_to_bin((uint8_t)(hour_value & 0x7F));
        hour_value = (uint8_t)(hour_low | (hour_value & 0x80));
    }

    if (!is_24_hour) {
        int is_pm = (hour_value & 0x80) != 0;
        hour_value &= 0x7F;
        if (is_pm && hour_value < 12) {
            hour_value = (uint8_t)(hour_value + 12);
        }
        if (!is_pm && hour_value == 12) {
            hour_value = 0;
        }
    }

    out->second = second_value;
    out->minute = minute_value;
    out->hour = hour_value;
    out->day = day_value;
    out->month = month_value;
    out->year = (uint16_t)(2000 + year_value);
}