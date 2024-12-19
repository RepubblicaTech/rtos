# [OS Codename] roadmap

The current status and the future of this OS.
A build (`.iso` file) will be released every time a Milestone is completed. The various milestones _might_ be updated over the course of time.

## 1st Milestone

- [X] Bare bones (Limine and 64-bit kernel)

- [X] `printf` implementation (+ E9 port "debugging")

- [X] GDT
- [X] Interrupt handling (IDT, ISRs, IRQs)
- [X] PIC support
- [X] LAPIC/IOAPIC Initialization

- [ ] ACPI
  - [X] Get RSDP/RSDT information
  - [X] MADT (LAPIC initialization)
  - [ ] _we'll probably need more tables in the future_

- [ ] Memory
  - [X] Get memory map
  - [ ] Memory management
    - [X] PMM
      - [X] Allocation/heap (`malloc`, `free`)
    - [ ] VMM
      - [X] Paging
      - [ ] Mapping devices to memory
        - [X] LAPIC
        - [X] I/O APIC
        - [ ] _leave this in case of other devices to map_
      - [ ] Actual VMM stuff (allocating, freeing pages etc.)

- [ ] File system support (ISO 9660, maybe FAT32/EXTx)

- [ ] Microkernel environment ([Eleanore Semaphore](https://wiki.osdev.org/Eleanore_Semaphore))
- [ ] Jump to userspace

## 2nd Milestone
- [ ] Basic shell
  - [ ] (decent) Keyboard driver
  - [ ] some kind of PATH
  - [ ] basic commands
- [ ] Video modes
  - [ ] Set better screen resolutions other than limine's default
  - [ ] (MAYBE) cool graphical stuff

## 3rd Milestone
- [ ] GUI