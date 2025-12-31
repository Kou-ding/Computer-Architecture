#!/bin/bash

# gem5 Results Parser Script
# Extracts performance metrics from stats.txt files and generates CSV

OUTPUT_CSV="gem5_results_summary.csv"
RESULTS_DIR="spec_results"

echo "Parsing gem5 simulation results..."
echo ""

# Create CSV header
echo "Configuration,Cache_Line_Size,L1I_Size,L1I_Assoc,L1D_Size,L1D_Assoc,L2_Size,L2_Assoc,sim_seconds,CPI,L1D_Miss_Rate,L1I_Miss_Rate,L2_Miss_Rate,sim_insts" > $OUTPUT_CSV

# Counter for processed directories
count=0
failed=0

# Find all directories matching the pattern
for dir in ${RESULTS_DIR}/speclibm_*; do
    if [ -d "$dir" ]; then
        stats_file="$dir/stats.txt"
        
        if [ -f "$stats_file" ]; then
            # Extract configuration from directory name
            config_name=$(basename "$dir")
            
            # Parse cache parameters from directory name
            # Expected format: speclibm_cl64_l1i32kB_l1ia2_l1d32kB_l1da2_l2512kB_l2a4
            cacheline=$(echo $config_name | grep -oP 'cl\K[0-9]+')
            l1i_size=$(echo $config_name | grep -oP 'l1i\K[0-9]+[kKmMgG][bB]?')
            l1i_assoc=$(echo $config_name | grep -oP 'l1ia\K[0-9]+')
            l1d_size=$(echo $config_name | grep -oP 'l1d\K[0-9]+[kKmMgG][bB]?' | head -1)
            l1d_assoc=$(echo $config_name | grep -oP 'l1da\K[0-9]+')
            l2_size=$(echo $config_name | grep -oP 'l2\K[0-9]+[kKmMgG][bB]?' | head -1)
            l2_assoc=$(echo $config_name | grep -oP 'l2a\K[0-9]+')
            
            # Extract metrics from stats.txt
            sim_seconds=$(grep "^sim_seconds" "$stats_file" | awk '{print $2}')
            cpi=$(grep "^system.cpu.cpi" "$stats_file" | head -1 | awk '{print $2}')
            l1d_miss=$(grep "^system.cpu.dcache.overall_miss_rate::total" "$stats_file" | awk '{print $2}')
            l1i_miss=$(grep "^system.cpu.icache.overall_miss_rate::total" "$stats_file" | awk '{print $2}')
            l2_miss=$(grep "^system.l2.overall_miss_rate::total" "$stats_file" | awk '{print $2}')
            sim_insts=$(grep "^sim_insts" "$stats_file" | awk '{print $2}')
            
            # Handle cases where metrics might not be found (set to "N/A")
            [ -z "$sim_seconds" ] && sim_seconds="N/A"
            [ -z "$cpi" ] && cpi="N/A"
            [ -z "$l1d_miss" ] && l1d_miss="N/A"
            [ -z "$l1i_miss" ] && l1i_miss="N/A"
            [ -z "$l2_miss" ] && l2_miss="N/A"
            [ -z "$sim_insts" ] && sim_insts="N/A"
            
            # Write to CSV
            echo "$config_name,$cacheline,$l1i_size,$l1i_assoc,$l1d_size,$l1d_assoc,$l2_size,$l2_assoc,$sim_seconds,$cpi,$l1d_miss,$l1i_miss,$l2_miss,$sim_insts" >> $OUTPUT_CSV
            
            ((count++))
            echo "Processed: $config_name"
            
        else
            echo "Warning: stats.txt not found in $dir"
            ((failed++))
        fi
    fi
done

echo ""
echo "========================================="
echo "Results parsing completed!"
echo "Successfully processed: $count configurations"
echo "Failed/Missing: $failed configurations"
echo "Output file: $OUTPUT_CSV"
echo "========================================="
echo ""
echo "You can now import $OUTPUT_CSV into Excel or any spreadsheet software."
echo "Recommended Excel operations:"
echo "  - Use 'Text to Columns' if needed"
echo "  - Create pivot tables for analysis"
echo "  - Generate charts comparing configurations"