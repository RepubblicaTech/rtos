# [OS Codename] roadmap

The current status and the future of this OS.
A build (`.iso` file) will be released every time a Milestone is completed. The various milestones _might_ be updated over the course of time.

## 1st Milestone

- [X] Bare bones (Limine and 64-bit kernel)

- [X] `printf` implementation (+ E9 port "debugging")

- [X] GDT
- [X] Interrupt handling (IDT, ISRs, IRQs)
- [X] PIC support

- [ ] ACPI/MMIO
  - [X] Get RSDP/RSDT information
  - [ ] MADT (LAPIC initialization)

- [ ] Memory
  - [X] Get memory map
  - [ ] Memory management
    - [X] PMM
      - [X] Allocation/heap (`malloc`, `free`)
    - [ ] VMM
      - [X] Paging
      - [ ] Mapping devices to memory
        - [ ] LAPIC
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