#include "scheduler.h"

#include "isr.h"
#include "memory.h"

#define SCHED_TASK_STACK_SIZE 4096

typedef struct {
    int id;
    char name[SCHED_TASK_NAME_MAX];
    int active;
    uint32_t runs;
    uint32_t last_tick;
    uint32_t counter;
    uint32_t esp;
} sched_task_t;

static sched_task_t* task_table = 0;
static int next_id = 1;
static int rr_last_index = -1;
static int current_task_index = -1;
static uint32_t kernel_context_esp = 0;
static volatile sched_task_t* running_task = 0;

static void scheduler_task_entry() {
    while (1) {
        if (running_task && running_task->active) {
            ((sched_task_t*)running_task)->counter++;
        }

        for (volatile uint32_t spin = 0; spin < 50000; spin++) {
        }
    }
}

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

static int find_next_active_after(int start_index) {
    if (!task_table) {
        return -1;
    }

    for (int i = 1; i <= SCHED_MAX_TASKS; i++) {
        int idx = (start_index + i) % SCHED_MAX_TASKS;
        if (task_table[idx].active) {
            return idx;
        }
    }

    return -1;
}

static uint32_t build_initial_task_frame() {
    uint8_t* stack = (uint8_t*)kmalloc_aligned(SCHED_TASK_STACK_SIZE, 16);
    if (!stack) {
        return 0;
    }

    registers_t* frame = (registers_t*)(stack + SCHED_TASK_STACK_SIZE - sizeof(registers_t));

    frame->edi = 0;
    frame->esi = 0;
    frame->ebp = 0;
    frame->esp = 0;
    frame->ebx = 0;
    frame->edx = 0;
    frame->ecx = 0;
    frame->eax = 0;
    frame->int_no = 0;
    frame->err_code = 0;
    frame->eip = (uint32_t)scheduler_task_entry;
    frame->cs = 0x08;
    frame->eflags = 0x202;

    return (uint32_t)frame;
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
        task_table[i].esp = 0;
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
            uint32_t initial_esp = build_initial_task_frame();
            if (!initial_esp) {
                return -2;
            }

            task_table[i].active = 1;
            task_table[i].id = next_id++;
            str_copy_name(task_table[i].name, name);
            task_table[i].runs = 0;
            task_table[i].last_tick = 0;
            task_table[i].counter = 0;
            task_table[i].esp = initial_esp;
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
            if (current_task_index == i) {
                current_task_index = -1;
                running_task = 0;
            }
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
    if (current_task_index == idx) {
        current_task_index = -1;
        running_task = 0;
    }
    return 1;
}

void scheduler_stop_all() {
    if (!task_table) {
        return;
    }
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        task_table[i].active = 0;
    }
    current_task_index = -1;
    running_task = 0;
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

uint32_t scheduler_schedule(registers_t* current_regs, uint32_t tick_count) {
    if (!task_table || !current_regs) {
        return (uint32_t)current_regs;
    }

    if (current_task_index >= 0 && task_table[current_task_index].active) {
        task_table[current_task_index].esp = (uint32_t)current_regs;
        task_table[current_task_index].runs++;
        task_table[current_task_index].last_tick = tick_count;

        current_task_index = -1;
        running_task = 0;
        if (kernel_context_esp != 0) {
            return kernel_context_esp;
        }
        return (uint32_t)current_regs;
    }

    kernel_context_esp = (uint32_t)current_regs;

    if (scheduler_active_count() == 0) {
        return kernel_context_esp;
    }

    int next_index = find_next_active_after(rr_last_index);
    if (next_index < 0) {
        return kernel_context_esp;
    }

    rr_last_index = next_index;
    current_task_index = next_index;
    running_task = &task_table[next_index];
    return task_table[next_index].esp;
}