#include "scheduler.h"

#include "memory.h"

typedef struct sched_task sched_task_t;
typedef void (*task_step_fn)(sched_task_t*);

struct sched_task {
    int id;
    char name[SCHED_TASK_NAME_MAX];
    int active;
    uint32_t runs;
    uint32_t last_tick;
    uint32_t counter;
    task_step_fn step;
};

static sched_task_t* task_table = 0;
static int next_id = 1;
static int rr_last_index = -1;

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
    while (src[i] && i < (SCHED_TASK_NAME_MAX - 1)) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void generic_count_step(sched_task_t* task) {
    task->counter++;
}

static int find_active_by_name(const char* name) {
    if (!task_table) {
        return -1;
    }

    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (task_table[i].active && str_equals(task_table[i].name, name)) {
            return i;
        }
    }
    return -1;
}

void scheduler_init() {
    task_table = (sched_task_t*)kmalloc((uint32_t)(sizeof(sched_task_t) * SCHED_MAX_TASKS));
    if (!task_table) {
        return;
    }

    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        task_table[i].id = 0;
        task_table[i].name[0] = '\0';
        task_table[i].active = 0;
        task_table[i].runs = 0;
        task_table[i].last_tick = 0;
        task_table[i].counter = 0;
        task_table[i].step = 0;
    }
}

int scheduler_start_named_task(const char* name) {
    if (!task_table || !name || !name[0]) {
        return -1;
    }

    if (find_active_by_name(name) >= 0) {
        return -3;
    }

    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (!task_table[i].active) {
            task_table[i].active = 1;
            task_table[i].id = next_id++;
            str_copy_name(task_table[i].name, name);
            task_table[i].runs = 0;
            task_table[i].last_tick = 0;
            task_table[i].counter = 0;
            task_table[i].step = generic_count_step;
            return task_table[i].id;
        }
    }

    return -4;
}

int scheduler_stop_task_by_id(int id) {
    if (!task_table || id <= 0) {
        return 0;
    }

    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (task_table[i].active && task_table[i].id == id) {
            task_table[i].active = 0;
            return 1;
        }
    }

    return 0;
}

int scheduler_stop_task_by_name(const char* name) {
    int idx = find_active_by_name(name);
    if (idx < 0) {
        return 0;
    }
    task_table[idx].active = 0;
    return 1;
}

void scheduler_stop_all() {
    if (!task_table) {
        return;
    }
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        task_table[i].active = 0;
    }
}

int scheduler_list_tasks(sched_task_info_t* out, int max_items) {
    if (!task_table || !out || max_items <= 0) {
        return 0;
    }

    int n = 0;
    for (int i = 0; i < SCHED_MAX_TASKS && n < max_items; i++) {
        if (task_table[i].active) {
            out[n].id = task_table[i].id;
            str_copy_name(out[n].name, task_table[i].name);
            out[n].active = task_table[i].active;
            out[n].runs = task_table[i].runs;
            out[n].last_tick = task_table[i].last_tick;
            out[n].counter = task_table[i].counter;
            n++;
        }
    }
    return n;
}

int scheduler_active_count() {
    if (!task_table) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (task_table[i].active) {
            count++;
        }
    }
    return count;
}

void scheduler_on_tick(uint32_t tick_count) {
    if (!task_table) {
        return;
    }

    int active = scheduler_active_count();
    if (active == 0) {
        return;
    }

    for (int tries = 0; tries < SCHED_MAX_TASKS; tries++) {
        rr_last_index = (rr_last_index + 1) % SCHED_MAX_TASKS;
        if (!task_table[rr_last_index].active) {
            continue;
        }

        task_table[rr_last_index].runs++;
        task_table[rr_last_index].last_tick = tick_count;
        if (task_table[rr_last_index].step) {
            task_table[rr_last_index].step(&task_table[rr_last_index]);
        }
        break;
    }
}