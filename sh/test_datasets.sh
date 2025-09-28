#!/bin/bash

# Get the directory where the data is located
DATA_HOME="data"

# Define dataset names array
datasets=("Twitch" "Musae" "Github" "Youtube" "Orkut" "DBLP" "Reddit" "LiveJournal")

# Define file suffixes array
suffixes=(".in" "_beg_pos.bin" "_csr.bin" "_weight.bin" ".g" "3.csv" "5.csv" "7.csv")

# Flag variable to track if any files are missing
missing_files_found=false

echo "Checking dataset file integrity..."
echo "Target directory: $DATA_HOME"
echo "========================================"

# Counter for overall statistics
total_files=0
missing_count=0
existing_count=0

# Iterate through all datasets
for dataset in "${datasets[@]}"; do
    echo "Checking dataset: $dataset"
    dataset_missing=false
    
    # Iterate through all suffixes
    for suffix in "${suffixes[@]}"; do
        filename="${dataset}${suffix}"
        filepath="${DATA_HOME}/${filename}"
        ((total_files++))
        
        # Check if file exists
        if [[ ! -f "$filepath" ]]; then
            echo "  ❌ MISSING: $filename"
            missing_files_found=true
            dataset_missing=true
            ((missing_count++))
        else
            echo "  ✅ FOUND: $filename"
            ((existing_count++))
        fi
    done
    
    # Print dataset summary
    if [[ "$dataset_missing" == true ]]; then
        echo "  Status: INCOMPLETE"
    else
        echo "  Status: COMPLETE"
    fi
    echo "----------------------------------------"
done

# Print detailed summary report
echo "SUMMARY REPORT"
echo "========================================"
echo "Total files checked: $total_files"
echo "Files found: $existing_count"
echo "Files missing: $missing_count"
echo ""

# Final status with exit code
if [[ "$missing_files_found" == true ]]; then
    echo "❌ CHECK FAILED: Missing files detected. Please download all required files before testing."
    echo "Return code: 1"
    exit 1
else
    echo "✅ CHECK PASSED: All dataset files are present and complete."
    echo "Return code: 0"
    exit 0
fi