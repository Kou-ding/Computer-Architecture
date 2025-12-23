---
documentclass: report 
papersize: a4 
fontsize: 12pt
date: \today
lang: en
title: Αρχιτεκτονική Προηγμένων Υπολογιστών και Επιταχυντών Lab 2 Report
author:
- Δάιος Γρηγόριος - **ΑΕΜ** 10334   
- Παπαδάκης Κωνσταντίνος Φώτιος - **ΑΕΜ** 10371
toc: true # table of contents
numbersections: true
geometry: margin=2cm # modify left/right paper margins
mainfont: "Nimbus Roman" # open source "Times New Roman"
---

# Console Output

When running the software and hardware emulation we note significantly larger "Load Binary File to Alveo U200" times. This increase can be attributed to the need to run an internal HDL simulator capable of accurately showcasing the kernel's behavior.

## Software

![sw_emu](media/sw_emu.png)

\newpage

## Hardware 

![hw_emu](media/hw_emu.png)

# Kernels and Compute Units

The compute unit which the kernel instantiates runs around 2 times faster on the hardware emulation when compared to the software emulation. This means that the hardware-wise optimizations were able to achieve better performance even though it wasn't that impressive.

## Software

![sw_kernels](media/sw_kernels.png)

## Hardware 

![hw_kernels](media/hw_kernels.png)

\newpage


# Kernel Data Transfers

On the hardware emulation implementation we can see that the kernel transfers are comprised of 14487 reads and 592 writes. Bandwidth utilization is 78% on the writes and 70% on the reads. There is space for improvement.

## Hardware 

![hw_kernel_data_trans](media/hw_kernel_data_trans.png)

# Host Data Transfers

Here we see clearly what we implemented in our code. The host writes from CPU RAM (DRAM) to the FPGA's global memory the 2 matrices **A** and **B** and when the kernel concludes its execution the host can read the **C_filt** matrix to compare with the software reference in the host/testbench code.

## Software

![sw_data_trans](media/sw_host_data_trans.png)

\newpage

## Hardware

![hw_host_data_trans](media/hw_host_data_trans.png)

# Timeline

## Software

Entire timeline: 

![sw_timeline](media/sw_timeline.png){width=80%}

Zoomed in:

![sw_timeline_zoom](media/sw_timeline_zoom.png){width=80%}

\newpage

## Hardware 

Entire timeline:  

![hw_timeline](media/hw_timeline.png){width=90%}

Zoomed in at around 42 - 45 seconds:  

![hw_timeline_zoom](media/hw_timeline_zoom.png){width=90%}

# API Calls

## Hardware

![hw_api_calls](media/hw_api_calls.png)

\newpage

<!-- SUMMARY -->
# Zip Contents

- lab2.cpp  
    - Final form of lab2's kernel. Run on Vitis.
- tb_lab2.cpp
    - The host which manages the lab2's kernel. Run on Vitis.
- lab2.pdf
    - This report