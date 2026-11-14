#!/usr/bin/env bash

# Gather executables in the build/ folder
exes=( $(find ./build -maxdepth 1 -type f -executable) )

# Print the list of executables
echo "Select an executable to run:"
i=1
for exe in "${exes[@]}"; do
    echo "$i) $exe"
    ((i++))
done

# Get user's choice
read -p "Enter a number: " choice

# Run the selected executable
if [[ $choice -ge 1 && $choice -le ${#exes[@]} ]]; then
    "${exes[$choice-1]}"
else
    echo "Invalid choice."
fi