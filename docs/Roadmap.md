# [OS Codename] roadmap
The current status and the future of this OS.
A build (`.iso` file) will be released every time a Milestone is completed. The various milestones will be updated over the course of time.

## 1st Milestone
- [X] Bare bones (Limine and 64-bit kernel)
- [X] `printf` support (+ E9 port "debugging")
- [X] GDT
- [X] Interrupt handling (IDT, ISRs, IRQs)
- [X] PIC support

- [ ] Memory
    - [X] Get memory map
    - [ ] Memory management
        - [ ] PMM
        - [ ] VMM
    - [ ] Paging
    - [ ] Allocation/heap (`malloc`, `free`)
- [ ] File system support (FAT32/extX)
- [ ] Microkernel environment ([Eleanore Semaphore](https://wiki.osdev.org/Eleanore_Semaphore))
- [ ] Jump to userspace

## 2nd Milestone
