# Gem5 Assignment

In this assignment we are going to be System call Emulation (SE) where we focus on running a specific program in contrast with Full System (FS) mode where we emulate an entire operating system.

## Step 1: Downloading Gem5

In this assingment we utilize a ready built virtual machine which has the correct ubuntu version, the necessary dependencies and the gem5 tool preinstalled.

## Step 2: Hello World

Here the starter_se.py script configures the gem5 emulator by defining the following parameters:

| Parameter      | Value  |
| -------------- | ------ |
| CPU Type            |  Minor  |
| Operation Frequency | 1GHz  |
| Basic Units      |   |
| Caches |   |
| Memory | |

- CPU Type: Minor
    - 1 core 
    - Clock 1GHz
    - Voltage 3.3V
- Caches
    - Cache line size = 64 columns
    - L1 Instruction cache
    - L1 Data cache
    - Page Table Walker cache
- Memory
    - DDR3 1600 8x8
    - 2 memory channels
    - 2 GB

![hello-world](/media/hello-world)

| Parameter      | Value  |
| -------------- | ------ |
| sim_seconds    |    |
| sim_ticks      |   |
| sim_insts      |   |
| host_inst_rate |   |
| # committed instructions | |
| L1 data cache accesses | |
| L2 cache accesses | |

### The different CPU models

In the documentation we can explore and learn more about the [different cpu models](https://www.gem5.org/documentation/general_docs/cpu_models/) gem5 provides:

atomic

minor 

hpi

## Step 3: First Program

## Step 4: 