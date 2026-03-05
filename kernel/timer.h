#ifndef TIMER_H
#define TIMER_H

#include "types.h"

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} clock_time_t;

void timer_init(uint32_t hz);
void timer_irq_handler();

uint32_t timer_get_ticks();
uint32_t timer_get_uptime_seconds();
uint32_t timer_get_hz();

void timer_get_clock(clock_time_t* out);
void timer_set_clock(uint8_t hour, uint8_t minute, uint8_t second);

#endif