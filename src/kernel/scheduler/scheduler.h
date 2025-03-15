/*
        SMP-aware scheduler implementation with load balancing

        (C) RepubblicaTech 2025
*/

#ifndef SCHEDULER_H
#define SCHEDULER_H 1

#include <isr.h>
#include <memory/vmm.h>
#include <spinlock.h>
#include <stdint.h>

#define SCHED_TIME_SLICE    10
#define SCHED_MAX_PROCESSES 2048
#define MAX_CPUS            32

#define PROC_STACK_PAGES 4
#define PROC_STACK_SIZE  (1 * PFRAME_SIZE)

// Process states
typedef enum {
    PROC_STATUS_NEW,
    PROC_STATUS_RUNNING,
    PROC_STATUS_READY,
    PROC_STATUS_BLOCKED,
    PROC_STATUS_DEAD
} process_state_t;

// Process priorities
#define PRIORITY_LOW    0
#define PRIORITY_NORMAL 1
#define PRIORITY_HIGH   2
#define PRIORITY_RT     3

typedef struct process_t {
    registers_t registers_frame;

    size_t pid;
    uint8_t priority;      // Process priority (0-3)
    uint32_t cpu_affinity; // Preferred CPU (-1 for any)
    uint32_t assigned_cpu; // Currently assigned CPU

    process_state_t state;
    size_t time_slice;
    size_t runtime_stats; // Track how long process has run

    uint64_t *pml4;
} process_t;

// Per-CPU scheduler data
typedef struct cpu_scheduler_t {
    size_t current_pid; // Currently running process index
    process_t *processes[SCHED_MAX_PROCESSES]; // Per-CPU process array
    size_t pid_count;    // Number of processes on this CPU
    atomic_flag lock;    // Scheduler lock for this CPU
    uint32_t cpu_id;     // CPU identifier
    uint64_t idle_ticks; // Track idle time for load balancing
} cpu_scheduler_t;

// Global scheduler functions
void scheduler_init();
void scheduler_init_cpu(uint32_t cpu_id);
process_t *create_process(void (*entry)(), uint8_t priority);
void destroy_process(size_t pid);

// Per-CPU scheduler functions
process_t *get_current_process();
void process_handler(registers_t *registers_frame);

// Load balancing
uint32_t get_least_loaded_cpu();
void balance_load();

// LAPIC timer handler
void lapic_timer_tick_handler(registers_t *regs);

#endif