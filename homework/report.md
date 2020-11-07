# Lab 4: Multi-core and Locks

<p align=right>18307130024 Jimmy Tan</p>

[TOC]

## 1 Multi-processing Systems

This section is based on [ARMv8-A Programmer Guide](https://cs140e.sergio.bz/docs/ARMv8-A-Programmer-Guide.pdf).

### 1.1 Getting the information about the running core

Software may need to know which core its code is executing upon. `ARMv8` provides the *Multi-Processor Affinity Register* (`MPIDR_EL1`) to identify the core thus satisfying the demand. 

To configure a virtual machine, `EL2` and `EL3` can set `MPIDR_EL1` to different values at run-time. `MPIDR_EL3` is based on the physical core and its value cannot be modified.

### 1.2 Symmetric multi-processing

In symmetric multi-processing (`SMP`), each core has the same view of memory and of shared hardware. There is a layer of abstraction from the softwares' perspective because the operating system hides the complexity of scheduling the tasks within or between the cores.

There are several trade-offs on whether to spread the tasks on more or less cores:

* Energy. On one hand, scheduling tasks on fewer cores can provide more idle resources. On the other hand, to achieve the same performance of a single core, spreading tasks on multiple cores requires lower frequencies, thus saving the power.

* Interrupt handling. Interrupts can be handled by multiple cores, which can reduce interrupt latency and the time spent on context switching. Similarly, only allowing interrupts to be handled on a few certain cores can reduce complexity.

### 1.3 Timers

The System Timer Architecture involves a global system counter at a fixed clock frequency. There are also local timers, secure and insecure ones, and ones for virtualization purposes. To implement the timer interrupt, each channel has a comparator and generates a timer interrupt when the count of the global counter is greater than or equal the the comparator.

Although the frequency of the global timer is fixed, the step of incrementing can be modified, to either 10 to 100 per clock tick, or every 10 to 100 clock ticks. 

The `CNTFRQ_EL0` register reports the frequency of the system timer. The `CNTPCT_EL0` register reports the current count value. The `CNTKCTL_EL1` register controls whether `EL0` can access the system timer.

### 1.4 Synchronization

The `aarch64` instruction set has three instructions for synchronization:

* Load Exclusive (`LDXR`)
* Store Exclusive (`STXR`). The instruction gives a signal about whether the store completed successfully.
* Clear Exclusive access monitor (`CLREX`)

Although the architecture and the hardware support implementations of exclusive access, the programmers should keep the right behavior on the software level.

## 2 Homework

### 2.1 Multi-core Booting

### 2.2 Spin-locks in Multicore

### 2.3 Add locks