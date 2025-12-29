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

![gem5-logo](/gem5/media/gem5-logo.png)

In this assignment we are going to be System call Emulation (SE) where we focus on running a specific program in contrast with Full System (FS) mode where we emulate an entire operating system.

## Project Overview

```
📁 gem5
├── 📂 bash_scripts - Scripts used to automate running the various benchmarks
│   
├── 📂 hello_world
│   ├── 📂 stats
│   │   ├── 📄 config.ini
│   │   ├── 📄 config.ini
│   │   └── 📄 stats.txt
│   └── 🐍 starter_se.py - python gem5 running script
│
├── 📂 spec_cpu2006
│   ├── 📂 Results_1 - spec_cpu2006 benchmark results
│   ├── 📂 Results_2 - spec_cpu2006 benchmark results cpu-clock=1GHz
│   ├── 📂 Results_3 - spec_cpu2006 benchmark results cpu-clock=4GHz
│   ├── 📂 Results_4 - spec_cpu2006 benchmark results cpu-clock=4GHz DDR3_2133_x64
│   └── 📂 Results_5 - spec_cpu2006 benchmark results cache experimentation
│   
├── 📂 tri
│   ├── 📂 MinorCPU - Results with CPU model 1
│   │   ├── 📄 config.ini
│   │   ├── 📄 config.ini
│   │   └── 📄 stats.txt
│   ├── 📂 TimingSimpleCPU - Results with CPU model 2
│   │   ├── 📄 config.ini
│   │   ├── 📄 config.ini
│   │   └── 📄 stats.txt
│   ├── ▶️ tri.c - Triangular number finding program
│   └── ⚙️ Makefile
│
├── 📂 media
│   ├── 🖼️ png 1
│   ├── 🖼️ png 2
│   └── 🖼️ etc ...
│
├── ⚙️ Makefile - Compiles the report
├── 📝 gem5.md - The markdown version of the report
└── 📄 gem5.pdf - The pdf version to the report
```

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

sim_seconds                                  0.000035                       # Number of seconds simulated
sim_ticks                                    35144000                       # Number of ticks simulated
sim_insts                                        5027                       # Number of instructions simulated
host_inst_rate                                 155696                       # Simulator instruction rate (inst/s)
system.cpu_cluster.cpus.committedInsts           5027                       # Number of instructions committed
system.cpu_cluster.cpus.dcache.demand_accesses::total         2160                       # number of demand (read+write) accesses
system.cpu_cluster.l2.demand_accesses::total          474                       # number of demand (read+write) accesses

```

Resulting table:

| Parameter                  | Value    |
| -------------------------- | -------- |
| sim_seconds                | 0.000035 |
| sim_ticks                  | 35144000 |
| sim_insts                  | 5027     |
| host_inst_rate             | 155696   |
| committedInsts             | 5027     |
| dcache.demand_accesses     | 2160     |
| l2.demand_accesses         | 474      |

An alternative way to calculate the number of data L1 and L2 cache accesses would be to:
- L1 data: Through the miss rate and the total misses.

```
system.cpu_cluster.cpus.dcache.overall_misses::total          177                       # number of overall misses
system.cpu_cluster.cpus.dcache.overall_miss_rate::total     0.081944                       # miss rate for overall accesses

```

$$\frac{177}{0.081944}=2160.0117$$

- L2: Through the L1 data and instruction cache misses. When the program can't find the information on the L1 cache the next step is to search L2. From this sum we also have to subtract the MSHRs hits(Miss Status Holding Registers) which represent the times a line was being fetched to L1 from a previous request.  

```
system.cpu_cluster.cpus.icache.demand_misses::total          327                       # number of demand (read+write) misses
system.cpu_cluster.cpus.dcache.demand_misses::total          177                       # number of demand (read+write) misses
system.cpu_cluster.cpus.dcache.demand_mshr_hits::total           30                       # number of demand (read+write) MSHR hits
```

$$ 327+177-30=474 $$



### The different CPU models

In the documentation we can explore and learn more about the [different cpu models](https://www.gem5.org/documentation/general_docs/cpu_models/) gem5 provides:

- SimpleCPU
    - A purely functional, in-order CPU model that is suited for cases where a detailed model is not necessary. Its subcategories include: BaseSimpleCPU, AtomicSimpleCPU and TimingSimpleCPU.
        - BaseSimpleCPU: The base the next two CPU models use.
        - AtomicSimpleCPU: Uses atomic memory accesses. It uses the latency estimates from the atomic accesses to estimate overall cache access time.
        - TimingSimpleCPU: Uses timing memory accesses. It stalls on cache accesses and waits for the memory system to respond prior to proceeding.
        
- O3CPU
    - An out of order CPU model loosely based on the Alpha 21264. It follows 5 pipeline stages. 
        - fetches the instractions, 
        - decodes them, 
        - renames them using a physical register file,
        - handles dispatching instructions to the instruction queue, telling the instruction queue to issue instruction, and executing and writing back instructions
        - commits instructions.
    It is very timing accurate.
- TraceCPU
    - Utilizes dependency and timing annotated traces called "elastic traces" developed for single threaded benchmarks. They are generated by the Elastic Trace Probe which is attached to the O3 CPU model. Its mission is to achieve memory-system (cache-hierarchy, interconnects and main memory) performance exploration in a fast and reasonably accurate way. Comparatively to the O3 CPU Model, TraceCPU manages significantly faster times. 
- MinorCPU
    - An in-order processor model with a fixed pipeline but configurable data structures and execute behaviour. Its predominant use case is to model processors with strict in-order execution behaviour. Moreover it allows visualisation of an instruction’s position in the pipeline through the MinorTrace/minorview.py format/tool. The intention is to provide a framework for micro-architecturally correlating the model with a particular, chosen processor with similar capabilities.

## Step 3: First Program

The next step has us create a simple C-lang trinagular number generator program and cross compiling it to run on the gem5 simulator. A triangular number can be defined as:

$$ T_N = \frac{(N+1)*N}{2} $$

### Cross-compilation 

We currently are edveloping a program on an x86 based processor. To compile this program to be able to run on an arm machine we make use of the gcc arm cross-compiler.

```bash
# Installing cross compilers
sudo apt install gcc-arm-linux-gnueabihf
sudo apt install g++-arm-linux-gnueabihf
```

The [Makefile](/gem5/tri/Makefile) we created for this program includes a **make arm** option. It uses the compiler we installed previously and the **--static** flag since we must incorporate all of the program's dependencies inside the executable.

```bash
# Running the simulation, CPU Model 1
./build/ARM/gem5.opt configs/example/se.py 
    --cpu-type=MinorCPU 
    --caches -c "/home/arch/Downloads/tri" --options "7"
# Running the simulation, CPU Model 2
./build/ARM/gem5.opt configs/example/se.py 
    --cpu-type=TimingSimpleCPU 
    --caches -c "/home/arch/Downloads/tri" --options "7"
```

| | MinorCPU | TimingSimpleCPU |
|-|----------|-----------------|
|sim_seconds|0.000038|0.000042|
|sim_ticks|37556000|42098000|
|sim_insts|8920|8863|
|host_inst_rate|129333|628622|
|committedInsts|8920|8863|
|dcache.demand_accesses|3255|2962|
|l2.demand_accesses|-|-|

First of all, the using the se.py python script doesn't create L2 cache. Other than that we notice three things; MinorCPU is slightly faster, accesses L1 data cache more times and and its simulation's instruction rate is a lot lower. 

TimingSimpleCPU takes longer to complete because it stalls on cache accesses and waits for the memory system to respond before proceeding. It is also a more computationally intensive model to simulate which explains the higher host instruction rate. 

The two models function differently in terms of accesses MinorCPU uses

## Step 4: SPEC CPU2006 Benchmark

SPEC CPU2006 is a benchmarking suite that stresses a system's:

- CPU
- Memory
- Compiler

It is trying to emulate computationally intensive real usage scenarios.

To run the following 5 benchmarks of the spec_cpu2006 suite:

- specbzip
- spechmmer
- speclibm
- specmfc
- specsjeng

we execute the [run_benchmarks.sh](/gem5/bash_scripts/run_benchmarks.sh) script.

```bash
bash run_benchmarks.sh 
```

To gather the important information derived from these benchmarks we use the script [read_results.sh](/gem5/bash_scripts/read_results.sh) which accelerates the gathering process while [stats_finder.ini](/gem5/bash_scripts/stats_finder.ini) specifies the:
- benchmarks we include in our report,
- parameters we are searching for in the stats.txt and
- output file name.

The script and .ini file location has to be inside the folder containing all the benchmark results. The script then searching for the **stats.txt** files in each results folder, traverses them, attains the relevant inforamtion and writes them in a text file. This can later be imported to excel to generate graphs.

```bash
~/Desktop/gem5/spec_results
```

### Benchmark Results

Some basic memory information that can be found on config.ini (or config.json) include:

|Attributes     |  specbzip   |  spechmmer  |  speclibm   |   specmfc   |  specsjeng  |
|---------------|-------------|-------------|-------------|-------------|-------------|
|cache_line_size| 64          | 64          | 64          | 64          | 64          |
|l1.dcache.size | 65536       | 65536       | 65536       | 65536       | 65536       |
|l1.dcache.assoc| 2           | 2           | 2           | 2           | 2           |
|l1.icache.size | 32768       | 32768       | 32768       | 32768       | 32768       |
|l1.icache.assoc| 2           | 2           | 2           | 2           | 2           |
|l2.cache.size  | 2097152     | 2097152     | 2097152     | 2097152     | 2097152     |
|l2.cache.size  | 8           | 8           | 8           | 8           | 8           |
|dram.type      |DDR3_1600_x64|DDR3_1600_x64|DDR3_1600_x64|DDR3_1600_x64|DDR3_1600_x64| 

To figure out the dram type, which is not explicitly states we can use the following values to produce it.

$$device\_bus\_width \times devices\_per\_rank = bus\_bits\Rightarrow$$

$$\Rightarrow 8 \times 8 = 64-bit bus $$
tCK = 1250ps

$$ freq = \frac{1}{tCK} = \frac{1}{10^{-12}\times1250} = 800 MHz $$
But since DDR transfers data on both edges this is doubled.

$$ burst\_length = 8 \Rightarrow \text{typical of DDR3} $$

Alternatively, tracing our steps from [se.py](spec_cpu2006/se.py) to [MemConfig.py](spec_cpu2006/MemConfig.py) and finally to [Options.py](spec_cpu2006/Options.py) we can clearly see that the default value, when not specifying the memory type with the relevant flag, is DDR3_1600_8x8.

Here can be seen the:

- CPI (Cycles Per Instruction)
- Simulated seconds
- dcache miss rate
- icache miss rate 
- l2 cache miss rate
- Total Instructions

![res1_seconds](spec_cpu2006/Results_1/res1_seconds.png)
![res1_cpi](spec_cpu2006/Results_1/res1_cpi.png)
![res1_dcache](spec_cpu2006/Results_1/res1_dcache.png)
![res1_icache](spec_cpu2006/Results_1/res1_icache.png)
![res1_l2cache](spec_cpu2006/Results_1/res1_l2cache.png)
![res1_insts](spec_cpu2006/Results_1/res1_insts.png)

Comments: ///////////////////////**TO-DO**///////////////////////////////

Afterwards we execute the same automation script adding the flags "--cpu-clock=1GHz", "--cpu-clock=2GHz". It is important to note that up to this point the default cpu-clock value has been 500 ticks $\Rightarrow$ 2GHz.

### CPU Clock 1GHz

| Constant parameters |   Value   |
| ------------------- | --------- |
| Instructions        | 100000000 |
| System Clock        | 1000      |
| CPU Clock           | 1000      |

![res2_seconds](spec_cpu2006/Results_2/res2_seconds.png)
![res2_cpi](spec_cpu2006/Results_2/res2_cpi.png)
![res2_dcache](spec_cpu2006/Results_2/res2_dcache.png)
![res2_icache](spec_cpu2006/Results_2/res2_icache.png)
![res2_l2cache](spec_cpu2006/Results_2/res2_l2cache.png)


### CPU Clock 4GHz

| Constant parameters |   Value   |
| ------------------- | --------- |
| Instructions        | 100000000 |
| System Clock        | 1000      |
| CPU Clock           | 250       |

![res3_seconds](spec_cpu2006/Results_3/res3_seconds.png)
![res3_cpi](spec_cpu2006/Results_3/res3_cpi.png)
![res3_dcache](spec_cpu2006/Results_3/res3_dcache.png)
![res3_icache](spec_cpu2006/Results_3/res3_icache.png)
![res3_l2cache](spec_cpu2006/Results_3/res3_l2cache.png)

Why are there different frequencies?
When changing the CPU clock speed this only affects the cpu's clock. The global/system clock remains the same. The reason for this split is due to the fact that we don't dont need such a high frequency clock for all procedures. For example the memory bus which inherits the system's frequency is usually run at lower frequencies 1600MHz. This conserves energy by having  
///////////////////////**TO-DO**///////////////////////////////

What would the cpu frequency be if we added a second cpu?
///////////////////////**TO-DO**///////////////////////////////

Is there perfect scaling in different clock systems? If not why?
///////////////////////**TO-DO**///////////////////////////////

Through the stats.txt findings we can confim cpu clock speed that we set:

- system.cpu_clk_domain.clock = 1000
- system.cpu_clk_domain.clock = 250

$$ T = 250 \times 10^{-12} and T = 1000 \times 10^{-12} $$
$$ f = \frac{1}{T} = 4GHz and f = \frac{1}{T} = 1GHz $$


### Memory type DDR3_2133_x64 (and CPU Clock 4GHz)

![res4_seconds](spec_cpu2006/Results_4/res4_seconds.png)
![res4_cpi](spec_cpu2006/Results_4/res4_cpi.png)
![res4_dcache](spec_cpu2006/Results_4/res4_dcache.png)
![res4_icache](spec_cpu2006/Results_4/res4_icache.png)
![res4_l2cache](spec_cpu2006/Results_4/res4_l2cache.png)
![res4_insts](spec_cpu2006/Results_4/res4_insts.png)

Observations:
///////////////////////**TO-DO**///////////////////////////////

## Step 4: Performance Optimization

At this point we will be tweaking the following values:

- Cache line size
- L1 Instruction cache size
- L1 Instruction cache associativity
- L1 Data cache
- L1 Data associativity
- L2 cache size
- L2 associativity

We start off by setting some restrictions sourcing from a theoretical standpoint:

- L1 cache <= 256KBytes
- 

To conduct our experiments we fully automate the procedure using a script that combines our previously created scripts.

## Step 5: Cost Function as a means for Performance Optimization


## Sources
[1] https://www.gem5.org/documentation/general_docs/cpu_models/SimpleCPU
[2] https://www.gem5.org/documentation/general_docs/cpu_models/O3CPU
[3] https://www.gem5.org/documentation/general_docs/cpu_models/TraceCPU
[4] https://www.gem5.org/documentation/general_docs/cpu_models/minor_cpu
[5] https://en.wikipedia.org/wiki/Double_data_rate
[6] 