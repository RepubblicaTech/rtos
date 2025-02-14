#include "scheduler.h"

#include <stddef.h>
#include <stdio.h>

#include <memory/heap/liballoc.h>
#include <memory/pmm.h>

#include <util/string.h>
#include <util/util.h>

#include <time.h>

#include <cpu.h>
#include <gdt.h>

#include <irq.h>
#include <mmio/apic/io_apic.h>

#include <spinlock.h>

size_t procs_counter;
size_t current_pid; // which process is currently running
process_t **processes;

process_t *get_current_process() {
    return processes[current_pid];
}

atomic_flag SCHEDULER_LOCK;

// to create a process, you need to give it some kind of starting point
process_t *create_process(void (*entry)()) {
    spinlock_acquire(&SCHEDULER_LOCK);

    process_t *process = kmalloc(sizeof(process_t));

    process->pml4 = (uint64_t *)pmm_alloc_page();
    pagemap_copy_to(process->pml4);
    copy_range_to_pagemap(process->pml4, get_kernel_pml4(), (uint64_t)processes,
                          sizeof(process_t) * SCHED_MAX_PROCESSES);

    // yeah... this looks very ugly but ehh... i need that liballoc maj
    // struct :/
    copy_range_to_pagemap(process->pml4, get_kernel_pml4(), 0x1000, 0x10000);

    memset(&process->registers_frame, 0, sizeof(registers_t));
    process->registers_frame.ds = GDT_DATA_SEGMENT;
    process->registers_frame.ss = GDT_DATA_SEGMENT;
    process->registers_frame.cs = GDT_CODE_SEGMENT;

    // the stack grows downwards :^)
    process->registers_frame.rsp =
        ((uint64_t)PHYS_TO_VIRTUAL(pmm_alloc_pages(PROC_STACK_PAGES))) +
        (PROC_STACK_SIZE);

    process->registers_frame.rip    = (uint64_t)entry;
    process->registers_frame.rflags = 0x202;

    process->time_slice = SCHED_TIME_SLICE;
    process->state      = PROC_STATUS_NEW;
    process->pid        = procs_counter;

    // append the process to the list
    processes[procs_counter++] = process;

    // debugf_ok("Process (PID: %llu, entry:%#llx) has been created\n",
    //           process->pid, process->registers_frame.rip);

    spinlock_release(&SCHEDULER_LOCK);

    return process;
}

void destroy_process(size_t pid) {
    // last process' pid is still (procs_counter - 1)
    if (pid >= procs_counter)
        return;

    process_t *process = processes[pid];
    if (!pid)
        return;

    memset(process->pml4, 0, PFRAME_SIZE);
    pmm_free(process->pml4, PROC_STACK_PAGES);

    void *a_rbp = (void *)(ROUND_UP(process->registers_frame.rbp, PFRAME_SIZE));
    pmm_free(a_rbp, PROC_STACK_PAGES);

    processes[pid] = NULL;

    if (pid == procs_counter - 1)
        procs_counter--;
}

void process_handler(registers_t *cur_registers_frame) {
    process_t *current_process = processes[current_pid];

    if (!current_process)
        return;

    if (current_process->state == PROC_STATUS_RUNNING) {
        memcpy(&current_process->registers_frame, cur_registers_frame,
               sizeof(registers_t));
    }

    if (current_process->time_slice > 0) {
        current_process->time_slice--;
        return;
    }

    current_pid = (current_pid + 1) % procs_counter;

    process_t *next_process = processes[current_pid];

    if (next_process) {
        next_process->state = PROC_STATUS_RUNNING;
        memcpy(cur_registers_frame, &next_process->registers_frame,
               sizeof(registers_t));
        _load_pml4(next_process->pml4);
    }
}

void scheduler_init() {
    processes = kcalloc(SCHED_MAX_PROCESSES, sizeof(process_t));
    memset(processes, 0, sizeof(process_t) * SCHED_MAX_PROCESSES);

    procs_counter = 0;
    current_pid   = 0;

    irq_registerHandler(0, sched_timer_tick);
}
