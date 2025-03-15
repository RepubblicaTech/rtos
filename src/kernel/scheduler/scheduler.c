#include "scheduler.h"
#include "isr.h"
#include "memory/vmm.h"
#include "spinlock.h"

#include <cpu.h>
#include <gdt.h>
#include <irq.h>
#include <memory/heap/liballoc.h>
#include <memory/pmm.h>
#include <mmio/apic/apic.h>
#include <mmio/apic/io_apic.h>
#include <smp/smp.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <util/string.h>
#include <util/util.h>

// Global scheduler variables
size_t global_pid_counter = 1;
atomic_flag PID_LOCK      = ATOMIC_FLAG_INIT;
cpu_scheduler_t cpu_schedulers[MAX_CPUS];

// Get the scheduler instance for the current CPU
cpu_scheduler_t *get_cpu_scheduler() {
    uint32_t cpu_id = lapic_get_id();
    return &cpu_schedulers[cpu_id];
}

// Get the current process running on this CPU
process_t *get_current_process() {
    cpu_scheduler_t *scheduler = get_cpu_scheduler();

    if (scheduler->current_pid >= scheduler->pid_count)
        return NULL;

    return scheduler->processes[scheduler->current_pid];
}

// Find CPU with smallest process queue
uint32_t get_least_loaded_cpu() {
    uint32_t least_loaded = 0;
    size_t min_processes  = cpu_schedulers[0].pid_count;

    for (uint32_t i = 1; i < cpu_count; i++) {
        if (cpu_schedulers[i].pid_count < min_processes) {
            min_processes = cpu_schedulers[i].pid_count;
            least_loaded  = i;
        }
    }

    return least_loaded;
}

// Create a new process with specified priority
process_t *create_process(void (*entry)(), uint8_t priority) {
    asm("cli");
    if (priority > PRIORITY_RT)
        priority = PRIORITY_NORMAL;

    process_t *process = kmalloc(sizeof(process_t));
    if (!process) {
        kprintf("Error: Failed to allocate memory for process\n");
        asm("sti");
        return NULL;
    }

    // Setup memory management for the process
    process->pml4 = (uint64_t *)pmm_alloc_page();
    if (!process->pml4) {
        kprintf("Error: Failed to allocate memory for process PML4\n");
        kfree(process);
        asm("sti");
        return NULL;
    }

    pagemap_copy_to(process->pml4);

    copy_range_to_pagemap(process->pml4, get_kernel_pml4(), 0x1000, 0x10000);

    // Setup registers
    memset(&process->registers_frame, 0, sizeof(registers_t));
    process->registers_frame.ds = GDT_DATA_SEGMENT;
    process->registers_frame.ss = GDT_DATA_SEGMENT;
    process->registers_frame.cs = GDT_CODE_SEGMENT;

    // Allocate and setup stack with 16-byte alignment
    // Allocate stack pages
    void *stack_physical = pmm_alloc_pages(PROC_STACK_PAGES);
    if (!stack_physical) {
        kprintf("Error: Failed to allocate stack pages for process\n");
        pmm_free(process->pml4, PROC_STACK_PAGES);
        kfree(process);
        asm("sti");
        return NULL;
    }

    // Convert to virtual address and calculate stack top
    uint64_t stack_virtual = PHYS_TO_VIRTUAL(stack_physical);
    uint64_t stack_top     = stack_virtual + PROC_STACK_SIZE;

    // Ensure 16-byte alignment for the stack pointer
    process->registers_frame.rsp = stack_top & ~0xFULL;
    process->registers_frame.rbp = process->registers_frame.rsp;

    process->registers_frame.rip    = (uint64_t)entry;
    process->registers_frame.rflags = 0x202;

    // Process attributes
    process->priority = priority;
    process->time_slice =
        SCHED_TIME_SLICE + (priority * 5); // More time for higher priority
    process->runtime_stats = 0;
    process->cpu_affinity  = -1; // No CPU affinity by default

    // Assign PID atomically
    spinlock_acquire(&PID_LOCK);
    process->pid = global_pid_counter++;
    spinlock_release(&PID_LOCK);

    // Find least loaded CPU and assign process to it
    uint32_t target_cpu   = get_least_loaded_cpu();
    process->assigned_cpu = target_cpu;

    // Add process to target CPU's run queue
    cpu_scheduler_t *scheduler = &cpu_schedulers[target_cpu];
    spinlock_acquire(&scheduler->lock);

    process->state = PROC_STATUS_READY;

    scheduler->processes[scheduler->pid_count] = process;
    scheduler->pid_count++;

    debugf_debug("Process %llu created and assigned to CPU %u. Entry:%#llx\n",
                 process->pid, target_cpu, process->registers_frame.rip);

    spinlock_release(&scheduler->lock);

    // If this is a different CPU, send IPI to notify it

    send_ipi_all(IPI_VECTOR_RESCHEDULE);

    asm("sti");

    return process;
}

// Destroy a process
void destroy_process(size_t pid) {
    // Find which CPU has this process
    for (uint32_t cpu_id = 0; cpu_id < cpu_count; cpu_id++) {
        cpu_scheduler_t *scheduler = &cpu_schedulers[cpu_id];
        spinlock_acquire(&scheduler->lock);

        for (size_t i = 0; i < scheduler->pid_count; i++) {
            if (scheduler->processes[i] &&
                scheduler->processes[i]->pid == pid) {
                process_t *process = scheduler->processes[i];

                // Free process resources
                memset((void *)PHYS_TO_VIRTUAL(process->pml4), 0, PFRAME_SIZE);
                pmm_free(process->pml4, 1); // Free PML4 (1 page)

                // Calculate stack base correctly
                // First get the base of the stack (rsp points to the top)
                uint64_t stack_top    = process->registers_frame.rsp;
                uint64_t stack_base   = stack_top - PROC_STACK_SIZE;
                // Convert to physical address for pmm_free
                void *phys_stack_base = (void *)VIRT_TO_PHYSICAL(stack_base);
                pmm_free(phys_stack_base, PROC_STACK_PAGES);

                // Remove from queue by shifting remaining elements
                for (size_t j = i; j < scheduler->pid_count - 1; j++) {
                    scheduler->processes[j] = scheduler->processes[j + 1];
                }
                scheduler->pid_count--;

                // If this was the current running process, adjust current_pid
                if (scheduler->current_pid == i) {
                    scheduler->current_pid = 0;
                }
                // If current_pid was after the deleted process, adjust it
                else if (scheduler->current_pid > i) {
                    scheduler->current_pid--;
                }

                kfree(process);
                spinlock_release(&scheduler->lock);
                return;
            }
        }

        spinlock_release(&scheduler->lock);
    }

    kprintf_warn("Process %llu not found for destruction\n", pid);
}

// Handle scheduling on a CPU
void process_handler(registers_t *regs) {
    _load_pml4(get_kernel_pml4());
    cpu_scheduler_t *scheduler = get_cpu_scheduler();
    spinlock_acquire(&scheduler->lock);
    if (scheduler->pid_count == 0) {
        scheduler->idle_ticks++;
        spinlock_release(&scheduler->lock);
        return;
    }

    // Save state of current process if one is running
    if (scheduler->current_pid < scheduler->pid_count) {
        process_t *current = scheduler->processes[scheduler->current_pid];
        if (current && current->state == PROC_STATUS_RUNNING) {
            // Save registers
            memcpy(&current->registers_frame, regs, sizeof(registers_t));
            current->state = PROC_STATUS_READY;

            // Update runtime stats
            current->runtime_stats++;

            // Check if time slice expired
            if (current->time_slice > 0) {
                current->time_slice--;

                // If time slice not expired, keep running this process
                if (current->time_slice > 0) {
                    current->state = PROC_STATUS_RUNNING;
                    spinlock_release(&scheduler->lock);
                    _load_pml4(current->pml4);
                    return;
                }

                // Reset time slice for next run based on priority
                current->time_slice =
                    SCHED_TIME_SLICE + (current->priority * 5);
            }
        }
    }

    // Check if any processes are in READY state
    bool ready_process_found = false;
    for (size_t i = 0; i < scheduler->pid_count; i++) {
        if (scheduler->processes[i] &&
            scheduler->processes[i]->state == PROC_STATUS_READY) {
            ready_process_found = true;
            break;
        }
    }

    // If no process is ready, keep current process running or idle
    if (!ready_process_found) {
        // If current process is valid, keep running it
        if (scheduler->current_pid < scheduler->pid_count &&
            scheduler->processes[scheduler->current_pid]) {
            process_t *current = scheduler->processes[scheduler->current_pid];
            current->state     = PROC_STATUS_RUNNING;
            spinlock_release(&scheduler->lock);
            _load_pml4(current->pml4);
            return;
        }

        // Otherwise CPU is idle
        scheduler->idle_ticks++;
        spinlock_release(&scheduler->lock);
        return;
    }

    // Priority-based scheduling:
    // First, try to find a high priority process that's ready
    size_t next_pid      = (scheduler->current_pid + 1) % scheduler->pid_count;
    size_t orig_next     = next_pid;
    uint8_t highest_prio = 0;
    size_t highest_prio_pid = next_pid;

    // Find highest priority ready process
    do {
        process_t *proc = scheduler->processes[next_pid];
        if (proc && proc->state == PROC_STATUS_READY &&
            proc->priority > highest_prio) {
            highest_prio     = proc->priority;
            highest_prio_pid = next_pid;
        }
        next_pid = (next_pid + 1) % scheduler->pid_count;
    } while (next_pid != orig_next);

    // If we found a high-priority process, use it
    if (highest_prio > 0) {
        next_pid = highest_prio_pid;
    } else {
        // Otherwise round-robin to next ready process
        size_t attempts = 0;
        next_pid        = (scheduler->current_pid + 1) % scheduler->pid_count;

        // Safety check to avoid infinite loop - only check as many times as
        // processes
        while (attempts < scheduler->pid_count) {
            if (scheduler->processes[next_pid] &&
                scheduler->processes[next_pid]->state == PROC_STATUS_READY) {
                break;
            }
            next_pid = (next_pid + 1) % scheduler->pid_count;
            attempts++;
        }

        // If we couldn't find a ready process after checking all processes
        if (attempts >= scheduler->pid_count) {
            scheduler->idle_ticks++;
            spinlock_release(&scheduler->lock);

            return;
        }
    }

    // Switch to the selected process
    scheduler->current_pid  = next_pid;
    process_t *next_process = scheduler->processes[scheduler->current_pid];

    next_process->state = PROC_STATUS_RUNNING;
    memcpy(regs, &next_process->registers_frame, sizeof(registers_t));

    // Make sure we have a valid PML4 before trying to load it
    if (next_process->pml4 != NULL && (uint64_t)next_process->pml4 != 0) {
        _load_pml4(next_process->pml4);
    } else {
        // If PML4 is invalid, use kernel page tables instead
        _load_pml4(get_kernel_pml4());
        debugf_warn(
            "Process %llu has invalid PML4 (address %#llx), using kernel page "
            "tables\n",
            next_process->pid, (uint64_t)next_process->pml4);
    }
    spinlock_release(&scheduler->lock);

    return;
}

// LAPIC timer handler for scheduling
void lapic_timer_tick_handler(registers_t *regs) {

    asm("cli");
    // Ensure we're in kernel page tables for scheduler

    // Update global time
    set_ticks(get_current_ticks() + 1);

    // Do scheduling

    process_handler(regs);

    // Send EOI to acknowledge the interrupt

    asm("sti");
    lapic_send_eoi();
}

// Periodically balance load between CPUs
void balance_load() {
    // Find the most and least loaded CPUs
    uint32_t most_loaded  = 0;
    uint32_t least_loaded = 0;
    size_t max_proc       = 0;
    size_t min_proc       = SIZE_MAX;

    for (uint32_t i = 0; i < cpu_count; i++) {
        if (cpu_schedulers[i].pid_count > max_proc) {
            max_proc    = cpu_schedulers[i].pid_count;
            most_loaded = i;
        }
        if (cpu_schedulers[i].pid_count < min_proc) {
            min_proc     = cpu_schedulers[i].pid_count;
            least_loaded = i;
        }
    }

    // Only balance if significant imbalance exists
    if (max_proc > min_proc + 1) {
        cpu_scheduler_t *src = &cpu_schedulers[most_loaded];
        cpu_scheduler_t *dst = &cpu_schedulers[least_loaded];

        spinlock_acquire(&src->lock);

        // Find a process we can migrate (not the currently running one)
        for (size_t i = 0; i < src->pid_count; i++) {
            if (i != src->current_pid &&
                src->processes[i]->state == PROC_STATUS_READY) {
                spinlock_acquire(&dst->lock);

                // Move process to destination CPU
                process_t *proc                  = src->processes[i];
                dst->processes[dst->pid_count++] = proc;
                proc->assigned_cpu               = least_loaded;

                // Remove from source CPU
                for (size_t j = i; j < src->pid_count - 1; j++) {
                    src->processes[j] = src->processes[j + 1];
                }
                src->pid_count--;

                // Adjust source current_pid if needed
                if (src->current_pid > i) {
                    src->current_pid--;
                }

                kprintf("Migrated process %llu from CPU %u to CPU %u\n",
                        proc->pid, most_loaded, least_loaded);

                // Notify destination CPU
                send_ipi(least_loaded, IPI_VECTOR_RESCHEDULE);

                spinlock_release(&dst->lock);
                break;
            }
        }

        spinlock_release(&src->lock);
    }
}

// Initialize scheduler for a specific CPU
void scheduler_init_cpu(uint32_t cpu_id) {
    cpu_scheduler_t *scheduler = &cpu_schedulers[cpu_id];

    // Initialize this CPU's scheduler
    scheduler->current_pid = 0;
    scheduler->pid_count   = 0;
    scheduler->cpu_id      = cpu_id;
    scheduler->idle_ticks  = 0;

    // Clear process array
    memset(scheduler->processes, 0, sizeof(process_t *) * SCHED_MAX_PROCESSES);

    // Set up LAPIC timer for this CPU
    lapic_write_reg(LAPIC_TIMER_DIV_REG, 0x3); // Divide by 16
    lapic_write_reg(LAPIC_TIMER_REG,
                    0x20000 | LAPIC_TIMER_VECTOR); // Periodic mode & vector

    // Set initial count for ~1ms timer ticks
    // In a real system, this would be calibrated first
    lapic_write_reg(LAPIC_TIMER_INIT_CNT, 10000);

    // Register the timer handler
    apic_registerHandler(LAPIC_TIMER_VECTOR, lapic_timer_tick_handler);

    kprintf("Initialized scheduler for CPU %u\n", cpu_id);
}

// Initialize the global scheduler
void scheduler_init() {
    if (cpu_count > MAX_CPUS)
        cpu_count = MAX_CPUS;

    // Initialize global variables
    global_pid_counter = 1;

    // Initialize the bootstrap CPU's scheduler
    scheduler_init_cpu(lapic_get_id());

    kprintf("SMP Scheduler initialized for %u CPUs\n", cpu_count);
}