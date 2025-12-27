---
documentclass: report 
papersize: a4 
fontsize: 12pt
date: \today
lang: en
title: Αρχιτεκτονική Προηγμένων Υπολογιστών και Επιταχυντών Gem5 Report
author: Παπαδάκης Κωνσταντίνος Φώτιος - **ΑΕΜ** 10371
toc: true # table of contents
numbersections: true
geometry: margin=2cm # modify left/right paper margins
mainfont: "Liberation Serif"
---

# Gem5 Assignment

In this assignment we are going to be System call Emulation (SE) where we focus on running a specific program in contrast with Full System (FS) mode where we emulate an entire operating system.

## Step 1: Downloading Gem5

In this assingment we utilize a ready built virtual machine which has the correct ubuntu version, the necessary dependencies and the gem5 tool preinstalled.

## Step 2: Hello World

Starting off we run a simple "Hello world!" program using the starter_se.py script. This configures the gem5 simulator by defining the following parameters:

- CPU 
    - Type: Minor
    - Cores: 1  
    - Clock: 1GHz
    - Voltage: 3.3V
- Caches
    - Cache line size: 64 columns
    - L1 Instruction cache
    - L1 Data cache
    - L2 cache
    - Walk cache
- Memory
    - DDR3 1600 8x8
    - 2 memory channels
    - 2 GB
- Others
    - Off-chip memory bus

```bash
# Navigate inside your gem5 folder
cd ~/Desktop/gem5

# Command
./build/ARM/gem5.opt configs/example/arm/starter_se.py --cpu="minor" "tests/test-progs/hello/bin/arm/linux/hello"
```

![hello-world](media/hello-world.png)

Through the following files:

- stats.txt
- config.ini
- config.json

we attain detailed information regarding our system. Through these we cross-check the system info we derived from the python script.

```
stats.txt - relevant lines
--------------------------



```
| Parameter      | Value    |
| -------------- | ------   |
| sim_seconds    | 0.000035 |
| sim_ticks      | 35144000 |
| sim_insts      | 5027     |
| host_inst_rate | 155696   |
| # committed instructions | |
| L1 data cache accesses | |
| L2 cache accesses | |

### The different CPU models

In the documentation we can explore and learn more about the [different cpu models](https://www.gem5.org/documentation/general_docs/cpu_models/) gem5 provides:

- SimpleCPU
    - 
- O3CPU
    - 
- TraceCPU
    - 
- MinorCPU
    - 

## Step 3: First Program

Next step has us create a simple C-lang trinagular number generator program and cross compiling it to run on the gem5 simulator. A triangular number can be defined as:

$$ T_N = \frac{(N+1)*N}{2} $$


```bash
./build/ARM/gem5.opt configs/example/se.py 
    --cpu-type=MinorCPU 
    --caches -c "/home/arch/Downloads/tri" --options "7"
```

## Step 4: SPEC CPU2006 Benchmark

SPEC CPU2006 is a benchmarking suite that stresses a system's:

- CPU
- Memory
- Compiler

It is trying to emulate computationally intensive real usage scenarios.

The script [read_results.sh](/gem5/bash_scripts/read_results.sh) accelerates the process of retreiving the results while [stats_finder.ini](/gem5/bash_scripts/stats_finder.ini) specifies the:
- benchmarks we include in our report,
- parameters we are searching for in the stats.txt and
- output file name.

The script location is placed on the following folder:

```bash
~/Desktop/gem5/spec_results
```
