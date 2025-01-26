/*
        Round Robin scheduler implementation

        (C) RepubblicaTech 2025
*/

#ifndef SCHEDULER_H
#define SCHEDULER_H 1

#include <isr.h>

#include <stdint.h>

#include <memory/vmm.h>

#define SCHED_TIME_SLICE 10

typedef enum {
    PROC_STATUS_NEW,
    PROC_STATUS_RUNNING,
    PROC_STATUS_DEAD,
    PROC_STATUS_WAITING
} process_state_t;

typedef struct process_t {
    struct process_t *prev;

    registers_t *registers_frame;

    uint64_t pid;
    process_state_t state;

    uint64_t time_slice;

    vmm_context_t *vmm_ctx;

    struct process_t *next;
} process_t;

process_t *get_current_process();

process_t *create_process(void (*entry)());
void destroy_process(process_t *process);

void process_handler(registers_t *registers_frame);

void scheduler_init();

#endif