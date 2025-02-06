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

size_t procs_counter;
uint64_t current_pid; // which process is currently running
process_t **processes = NULL;

process_t *get_current_process() {
    return processes[current_pid];
}

atomic_flag SCHEDULER_LOCK;

// to create a process, you need to give it some kind of starting point
process_t *create_process(void (*entry)()) {
    spinlock_acquire(&SCHEDULER_LOCK);

    _load_pml4(get_kernel_pml4());

    process_t *process = kmalloc(sizeof(process_t));

    process->pml4 = (uint64_t *)PHYS_TO_VIRTUAL(pmm_alloc_page());
    pagemap_copy_to(process->pml4);

    memset(&process->registers_frame, 0, sizeof(registers_t));
    // process->registers_frame.ds = GDT_DATA_SEGMENT;
    process->registers_frame.ss = GDT_DATA_SEGMENT;
    process->registers_frame.cs = GDT_CODE_SEGMENT;
    // the stack grows downwards :^)
    process->registers_frame.rsp =
        (uint64_t)(PHYS_TO_VIRTUAL(pmm_alloc_pages(PROC_STACK_PAGES)) +
                   (PROC_STACK_SIZE - 1));
    process->registers_frame.rbp    = 0;
    process->registers_frame.rip    = (uint64_t)entry;
    process->registers_frame.rflags = 0x202;

    process->pid = procs_counter;

    process->time_slice = SCHED_TIME_SLICE;
    process->state      = PROC_STATUS_NEW;
    process->pid        = procs_counter++;

    // process->prev = NULL;
    // process->next = NULL;

    // append the process to the list
    processes[procs_counter - 1] = process;

    // debugf_debug("Process (PID: %llu, entry:%#llx) has been created\n",
    //              process->pid, process->registers_frame.rip);

    spinlock_release(&SCHEDULER_LOCK);

    return process;
}

void destroy_process(process_t *process) {
    if (!processes || !process)
        return;

    spinlock_acquire(&SCHEDULER_LOCK);

    _load_pml4(get_kernel_pml4());

    for (int i = 0; i <= procs_counter; i++) {
        if (processes[i] == process) {

            pmm_free(process->pml4, 1);
            pmm_free((void *)process->registers_frame.rbp - PROC_STACK_SIZE, 4);
            kfree(process);

            spinlock_release(&SCHEDULER_LOCK);
        }
    }

    kprintf_warn("No such process %p. Ignoring\n", process);
}

void process_handler(registers_t *cur_registers_frame) {
    if (!processes || !processes[current_pid])
        return;

    process_t *current_process = processes[current_pid];

    if (current_process->state == PROC_STATUS_RUNNING &&
        current_process->time_slice > 0) {
        current_process->time_slice--;
        memcpy(&current_process->registers_frame, cur_registers_frame,
               sizeof(registers_t));

        return;

    } else {
        current_process->state = PROC_STATUS_RUNNING;
    }

    // time slice is <= 0
    if (current_process->time_slice <= 0) {
        current_process->time_slice = SCHED_TIME_SLICE;
        current_pid                 = ++current_pid % procs_counter;
        current_process             = processes[current_pid];
        current_process->state      = PROC_STATUS_NEW;
    }

    memcpy(cur_registers_frame, &current_process->registers_frame,
           sizeof(registers_t));
    _load_pml4((uint64_t *)VIRT_TO_PHYSICAL(current_process->pml4));
}

void scheduler_init() {
    processes = kcalloc(SCHED_MAX_PROCESSES, sizeof(process_t));
    for (int i = 0; i < SCHED_MAX_PROCESSES; i++) {
        processes[i] = NULL;
    }

    procs_counter = 0;
    current_pid   = 0;

    irq_registerHandler(0, sched_timer_tick);
}
