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
process_t *current_process;

process_t *get_current_process() {
    return current_process;
}

atomic_flag SCHEDULER_LOCK;

// to create a process, you need to give it some kind of starting point
process_t *create_process(void (*entry)()) {
    spinlock_acquire(&SCHEDULER_LOCK);
    process_t *process          = kmalloc(sizeof(process_t));
    vmm_context_t *proc_vmm_ctx = vmm_ctx_init(NULL, VMO_KERNEL_RW);

    registers_t *cpu_frame = kmalloc(sizeof(registers_t));
    cpu_frame->ds          = GDT_DATA_SEGMENT;
    cpu_frame->ss          = cpu_frame->ds;
    cpu_frame->cs          = GDT_CODE_SEGMENT;
    // the stack grows downwards :^)
    cpu_frame->rsp = (uint64_t)((uint64_t)kmalloc(PMLT_SIZE) + (PMLT_SIZE - 1));
    cpu_frame->rip = (uint64_t)entry;
    cpu_frame->rflags = 0x202;

    process->pid             = pid_counter;
    process->registers_frame = cpu_frame;
    process->vmm_ctx         = proc_vmm_ctx;

    process->time_slice = SCHED_TIME_SLICE;
    process->state      = PROC_STATUS_NEW;
    process->pid        = pid_counter;

    pid_counter++;

    debugf_debug("Process (PID: %llu, entry:%#llx) has been created\n",
                 process->pid, process->registers_frame->rip);

    process->prev = NULL;
    process->next = NULL;

    // append the process to the list
    if (current_process != NULL) {
        process_t *last_process;
        for (last_process = current_process; last_process->next != NULL;
             last_process = last_process->next)
            ;

        if (last_process->next != NULL)
            process->next = last_process->next;
        last_process->next = process;
        process->prev      = last_process;
    } else {
        current_process = process;
    }

    spinlock_release(&SCHEDULER_LOCK);
    return process;
}

void destroy_process(process_t *process) {
    spinlock_acquire(&SCHEDULER_LOCK);

    kfree(process->registers_frame);
    process->registers_frame = NULL;

    vmm_ctx_destroy(process->vmm_ctx);

    kfree(process);

    spinlock_release(&SCHEDULER_LOCK);
}

void process_handler(registers_t *registers_frame) {
    // we need to switch the process as fast as possibile
    // if it's NULL then close immediately
    if (!current_process)
        return;

    // if it's running and it has enough time slice, just decrease it
    if (current_process->state == PROC_STATUS_RUNNING &&
        current_process->time_slice > 0) {
        current_process->time_slice--;

        memcpy(current_process->registers_frame, registers_frame,
               sizeof(registers_t));
    }

    // here, its time slice is 0
    // is there a next process?
    if (current_process->next) {
        current_process = current_process->next;
    }

    current_process->state = PROC_STATUS_RUNNING;

    // here, we should have an existing next
    memcpy(registers_frame, current_process->registers_frame,
           sizeof(registers_t));
    vmm_switch_ctx(current_process->vmm_ctx);
    vmm_init(current_process->vmm_ctx);
    _load_pml4((uint64_t *)current_process->vmm_ctx->pml4_table);
}

void scheduler_init() {
    current_process = NULL;
    pid_counter     = 0;

    if (check_apic()) {
        apic_register_handler(0, sched_timer_tick);
    } else {
        irq_registerHandler(0, sched_timer_tick);
    }
}
