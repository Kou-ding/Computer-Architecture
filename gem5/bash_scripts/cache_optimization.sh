#!/bin/bash

# gem5 Cache Parameter Sweep Script for SPEC LBM benchmark
# This script varies cache parameters and runs simulations

# Base configuration
GEM5_BIN="./build/ARM/gem5.opt"
CONFIG="configs/example/se.py"
BENCHMARK="spec_cpu2006/470.lbm/src/speclibm"
BENCH_ARGS="20 spec_cpu2006/470.lbm/data/lbm.in 0 1 spec_cpu2006/470.lbm/data/100_100_130_cf_a.of"
MAX_INSTS="100000000"

# Parameter ranges
CACHELINE_SIZES=(32 64 128)
L1I_SIZES=(16kB 32kB 64kB 128kB)
L1I_ASSOCS=(2 4 8)
L1D_SIZES=(16kB 32kB 64kB 128kB)
L1D_ASSOCS=(2 4 8)
L2_SIZES=(256kB 512kB 1MB 2MB)
L2_ASSOCS=(4 8)

# Counter for tracking simulations
sim_count=0

echo "Starting gem5 cache parameter sweep..."
echo "Total configurations to test: This will be calculated based on your selection"
echo ""

# Nested loops to iterate through all combinations
for cacheline in "${CACHELINE_SIZES[@]}"; do
  for l1i_size in "${L1I_SIZES[@]}"; do
    for l1i_assoc in "${L1I_ASSOCS[@]}"; do
      for l1d_size in "${L1D_SIZES[@]}"; do
        for l1d_assoc in "${L1D_ASSOCS[@]}"; do
          for l2_size in "${L2_SIZES[@]}"; do
            for l2_assoc in "${L2_ASSOCS[@]}"; do
              
              # Create unique directory name for this configuration
              output_dir="spec_results/speclibm_cl${cacheline}_l1i${l1i_size}_l1ia${l1i_assoc}_l1d${l1d_size}_l1da${l1d_assoc}_l2${l2_size}_l2a${l2_assoc}"
              
              # Increment counter
              ((sim_count++))
              
              echo "----------------------------------------"
              echo "Running simulation $sim_count"
              echo "Configuration:"
              echo "  Cache Line Size: ${cacheline}B"
              echo "  L1I Size: $l1i_size, Assoc: $l1i_assoc"
              echo "  L1D Size: $l1d_size, Assoc: $l1d_assoc"
              echo "  L2 Size: $l2_size, Assoc: $l2_assoc"
              echo "  Output: $output_dir"
              echo ""
              
              # Run gem5 simulation
              $GEM5_BIN -d $output_dir $CONFIG \
                --cpu-type=MinorCPU \
                --caches \
                --l2cache \
                --l1d_size=$l1d_size \
                --l1i_size=$l1i_size \
                --l2_size=$l2_size \
                --l1i_assoc=$l1i_assoc \
                --l1d_assoc=$l1d_assoc \
                --l2_assoc=$l2_assoc \
                --cacheline_size=$cacheline \
                --cpu-clock=1GHz \
                -c $BENCHMARK \
                -o "$BENCH_ARGS" \
                -I $MAX_INSTS
              
              # Check if simulation completed successfully
              if [ $? -eq 0 ]; then
                echo "Simulation $sim_count completed successfully"
              else
                echo "ERROR: Simulation $sim_count failed!"
              fi
              echo ""
              
            done
          done
        done
      done
    done
  done
done

echo "========================================="
echo "All simulations completed!"
echo "Total simulations run: $sim_count"
echo "Results stored in spec_results/speclibm_* directories"
echo "========================================="