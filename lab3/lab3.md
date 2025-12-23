---
documentclass: report 
papersize: a4 
fontsize: 12pt
date: \today
lang: en
title: Αρχιτεκτονική Προηγμένων Υπολογιστών και Επιταχυντών Lab 3 Report
author:
- Δάιος Γρηγόριος - **ΑΕΜ** 10334   
- Παπαδάκης Κωνσταντίνος Φώτιος - **ΑΕΜ** 10371
toc: true # table of contents
numbersections: true
geometry: margin=2cm # modify left/right paper margins
mainfont: "Nimbus Roman" # open source "Times New Roman"
---

<!-- INTRODUCTION -->
# Introduction

Lab3 improves on Lab2 by:

- Vectorizing the input/output decreasing the memory accesses
- Storing the input/output to different banks achieving parallelized memory access 



<!-- SUMMARY -->
# Zip Contents

- lab3.cpp  
    - Final form of lab3's kernel. Run on Vitis.
- tb_lab3.cpp
    - The host which manages the lab3's kernel. Run on Vitis.
- lab3.pdf
    - This report