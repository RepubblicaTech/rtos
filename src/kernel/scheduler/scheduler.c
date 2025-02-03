#include "scheduler.h"

#include <stddef.h>
#include <stdio.h>

#include <memory/heap/liballoc.h>
#include <memory/pmm.h>

#include <util/string.h>

#include <time.h>

#include <cpu.h>
#include <gdt.h>

#include <irq.h>
#include <mmio/apic/io_apic.h>

#include <spinlock.h>

uint64_t pid_counter;
uint64_t current_pid; // which process is currently running
process_t **processes = NULL;

process_t *get_current_process() {
    return processes[current_pid];
}

atomic_flag SCHEDULER_LOCK;

// to create a process, you need to give it some kind of starting point
process_t *create_process(void (*entry)()) {
    _load_pml4(get_kernel_pml4());

    process_t *process = kmalloc(sizeof(process_t));

    process->pml4 = (uint64_t *)PHYS_TO_VIRTUAL(pmm_alloc_page());
    pagemap_copy_to(process->pml4);

    registers_t cpu_frame;
    process->registers_frame.ds = GDT_DATA_SEGMENT;
    process->registers_frame.cs = GDT_CODE_SEGMENT;
    // the stack grows downwards :^)
    process->registers_frame.rsp =
        (uint64_t)(PHYS_TO_VIRTUAL(pmm_alloc_page()) + (PFRAME_SIZE - 1));
    process->registers_frame.rip    = (uint64_t)entry;
    process->registers_frame.rflags = 0x202;

    process->pid = pid_counter;

    process->time_slice = SCHED_TIME_SLICE;
    process->state      = PROC_STATUS_NEW;
    process->pid        = pid_counter;

    // process->prev = NULL;
    // process->next = NULL;

    // append the process to the list
    processes[pid_counter] = process;

    pid_counter++;

    // debugf_debug("Process (PID: %llu, entry:%#llx) has been created\n",
    //              process->pid, process->registers_frame.rip);

    return process;
}

void destroy_process(process_t *process) {
    spinlock_acquire(&SCHEDULER_LOCK);

    kfree(process);

    spinlock_release(&SCHEDULER_LOCK);
}

void process_handler(registers_t *cur_registers_frame) {
    if (!processes)
        return;

    // _load_pml4(get_kernel_pml4());
    process_t *current_process = processes[current_pid];
    if (!current_process)
        return;

    // time slice is <= 0
    if (current_process->time_slice <= 0) {
        current_process->time_slice = SCHED_TIME_SLICE;
        current_pid                 = ++current_pid % pid_counter;
        current_process             = processes[current_pid];
    }

    if (current_process->state == PROC_STATUS_RUNNING &&
        current_process->time_slice > 0) {
        current_process->time_slice--;
        memcpy(&processes[current_pid]->registers_frame, cur_registers_frame,
               sizeof(registers_t));

        return;

    } else {
        current_process->state = PROC_STATUS_RUNNING;
    }

    // here, we should have an existing next
    memcpy(cur_registers_frame, &processes[current_pid]->registers_frame,
           sizeof(registers_t));
    _load_pml4((uint64_t *)VIRT_TO_PHYSICAL(processes[current_pid]->pml4));
}

void scheduler_init() {
    processes = kcalloc(SCHED_MAX_PROCESSES, sizeof(process_t));
    for (int i = 0; i < SCHED_MAX_PROCESSES; i++) {
        processes[i] = NULL;
    }

    pid_counter = 0;
    current_pid = 0;

    irq_registerHandler(0, sched_timer_tick);
}
