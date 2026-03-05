#include "timer.h"

#include "io.h"
#include "rtc.h"

#define PIT_BASE_FREQUENCY 1193180

static volatile uint32_t timer_ticks = 0;
static volatile uint32_t uptime_seconds = 0;
static volatile uint32_t tick_hz = 100;
static volatile uint32_t subsecond_ticks = 0;

static volatile uint8_t clock_hour = 0;
static volatile uint8_t clock_minute = 0;
static volatile uint8_t clock_second = 0;

static void tick_clock_one_second() {
    clock_second++;
    if (clock_second < 60) {
        return;
    }
    clock_second = 0;

    clock_minute++;
    if (clock_minute < 60) {
        return;
    }
    clock_minute = 0;

    clock_hour++;
    if (clock_hour >= 24) {
        clock_hour = 0;
    }
}

void timer_init(uint32_t hz) {
    if (hz == 0) {
        hz = 100;
    }

    tick_hz = hz;

    rtc_datetime_t now;
    rtc_read_datetime(&now);
    clock_hour = now.hour;
    clock_minute = now.minute;
    clock_second = now.second;

    uint32_t divisor = PIT_BASE_FREQUENCY / tick_hz;
    if (divisor == 0) {
        divisor = 1;
    }

    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_irq_handler() {
    timer_ticks++;
    subsecond_ticks++;

    if (subsecond_ticks >= tick_hz) {
        subsecond_ticks = 0;
        uptime_seconds++;
        tick_clock_one_second();
    }
}

uint32_t timer_get_ticks() {
    return timer_ticks;
}

uint32_t timer_get_uptime_seconds() {
    return uptime_seconds;
}

uint32_t timer_get_hz() {
    return tick_hz;
}

void timer_get_clock(clock_time_t* out) {
    if (!out) {
        return;
    }
    out->hour = clock_hour;
    out->minute = clock_minute;
    out->second = clock_second;
}

void timer_set_clock(uint8_t hour, uint8_t minute, uint8_t second) {
    if (hour > 23 || minute > 59 || second > 59) {
        return;
    }
    clock_hour = hour;
    clock_minute = minute;
    clock_second = second;
}