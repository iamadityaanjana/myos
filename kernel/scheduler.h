#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

#define SCHED_MAX_TASKS 8
#define SCHED_TASK_NAME_MAX 16

typedef struct {
    int id;
    char name[SCHED_TASK_NAME_MAX];
    int active;
    uint32_t runs;
    uint32_t last_tick;
    uint32_t counter;
} sched_task_info_t;

void scheduler_init();
void scheduler_on_tick(uint32_t tick_count);

int scheduler_start_named_task(const char* name);
int scheduler_stop_task_by_id(int id);
int scheduler_stop_task_by_name(const char* name);
void scheduler_stop_all();

int scheduler_list_tasks(sched_task_info_t* out, int max_items);
int scheduler_active_count();

#endif