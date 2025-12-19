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

<!-- SW -->
# Software Emulation

## Console Output

![sw_emu](media/sw_emu.png)

## Kernels and Compute Units

![sw_kernels](media/sw_kernels.png)

\newpage

## Host Data Transfers

![sw_data_trans](media/sw_host_data_trans.png)

## Timeline

![sw_timeline](media/sw_timeline.png)

\newpage

<!-- HW -->
# Hardware Emulation

## Console Output

![hw_emu](media/hw_emu.png)

## Kernels and Compute Units

![hw_kernels](media/hw_kernels.png)

\newpage

## Host Data Transfers

![hw_host_data_trans](media/hw_host_data_trans.png)

## Kernel Data Transfers

![hw_kernel_data_trans](media/hw_kernel_data_trans.png)

\newpage

## Timeline

Entire timeline:  

![hw_timeline](media/hw_timeline.png){width=90%}

Zoomed in at around 42 - 45 seconds:  

![hw_timeline_zoom](media/hw_timeline_zoom.png){width=90%}

\newpage

## API Calls

![hw_api_calls](media/hw_api_calls.png)

\newpage

<!-- SUMMARY -->
# Zip Contents
- lab2test.cpp
    - Locally run testing program with fully bufferized matrices.
- lab2test_easy.cpp
    -  Locally run testing program, similar to lab2.cpp but doesn't utilize buffers for the calculation of **C_filter**.
- lab2.cpp  
    - Final form of lab2's kernel. Run on Vitis.
- tb_lab2.cpp
    - The host which manages the lab2's kernel. Run on Vitis.