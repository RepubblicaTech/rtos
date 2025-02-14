# [OS Codename] roadmap

The current status and the future of this OS.
A build (`.iso` file) will be released every time a Milestone is completed. The various milestones _might_ be updated over the course of time.

## 1st Milestone

- [X] Bare bones (Limine and 64-bit kernel)

- [X] `printf` implementation (+ E9 port "debugging")

- [X] GDT
- [X] Interrupt handling (IDT, ISRs, IRQs)
- [X] PIC support
- [ ] PIT Driver
  - [X] Initialization
  - [X] PIT-supported sleep
- [X] LAPIC/IOAPIC Initialization
  - [X] IRQ redirection to I/O APIC
  - [X] Interrupts work
  - [ ] LAPIC timer init

- [ ] ACPI
  - [X] Get RSDP/RSDT information
  - [X] MADT (LAPIC initialization)
  - [ ] HPET (maybe)
  - [ ] uACPI implementaion
  - [ ] _we'll probably need more tables in the future_

- [X] Memory
  - [X] Get memory map
  - [X] Memory management
    - [X] PMM
      - [X] Allocating/freeing physical memory
    - [X] VMM
      - [X] Paging
      - [X] Mapping devices to memory
        - [X] LAPIC
        - [X] I/O APIC
      - [X] Actual VMM stuff (allocating, freeing virtual memory etc.)  
    - [X] Kernel heap (`kmalloc`, `kfree`)

- [X] Scheduling

- [ ] File system support (ISO 9660, maybe FAT32/EXTx)

- [ ] SMP

- [ ] Microkernel environment ([Eleanore Semaphore](https://wiki.osdev.org/Eleanore_Semaphore))
- [ ] Jump to userspace

## 2nd Milestone
- [ ] ELF loading
- [ ] Basic shell
  - [ ] (decent) Keyboard driver
  - [ ] some kind of PATH
  - [ ] basic commands
- [ ] Video modes
  - [ ] Set better screen resolutions other than limine's default
  - [ ] (MAYBE) cool graphical stuff

## 3rd Milestone
- [ ] GUI