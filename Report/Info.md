# Parallel Sobel Edge Detection (C++ / OpenMP)
Parallel Sobel Edge Detection implemented using C++ and OpenMP. Includes a sequential baseline, fully parallelized Sobel filter, performance benchmarking, speedup &amp; efficiency analysis, cache/memory hierarchy evaluation, and report for Phase 1 of the SW401 Parallel &amp; Distributed Systems course.


███████╗ ██████╗ ██████╗ ███████╗██╗     
██╔════╝██╔═══██╗██╔══██╗██╔════╝██║     
███████╗██║   ██║██████╔╝█████╗  ██║     
╚════██║██║   ██║██╔══██╗██╔══╝  ██║     
███████║╚██████╔╝██║  ██║███████╗███████╗
╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝
   Parallel Sobel Edge Detection – SW401


A high-performance Sobel edge detection implementation written in **C++** with **OpenMP parallelization**, delivered as part of **Phase 1 – Parallel Foundations** of the **SW401 Parallel & Distributed Systems** course.

---

## Features
- **Sequential Implementation** (baseline)
- **OpenMP Parallel Version** using:
  - `#pragma omp parallel for`
  - `collapse(2)`
  - `dynamic` scheduling
- **Benchmark Automation**
  - Runtime measurement
  - Speedup calculation
  - Efficiency calculation
- **Plots**:
  - Speedup vs Threads
  - Efficiency vs Threads
- **Memory Hierarchy Evaluation**
  - Cache effects on large images
- **PGM (P2) format image support**
- **Ready-to-compile Makefile**
- **Phase 1 PDF report included**

---
