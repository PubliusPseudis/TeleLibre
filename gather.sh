#!/bin/bash

output_file="telelibre_project_combined.txt"

# Function to add a file to the output
add_file() {
    echo "=== $1 ===" >> "$output_file"
    echo >> "$output_file"
    cat "$1" >> "$output_file"
    echo >> "$output_file"
    echo >> "$output_file"
}

# Start with a clean file
> "$output_file"

# Add CMakeLists.txt
add_file "CMakeLists.txt"

# Add header files
for file in include/*.h; do
    add_file "$file"
done

# Add source files
for file in src/*.cpp; do
    add_file "$file"
done

# Add any other important files (add more as needed)
# add_file "other_important_file.txt"

echo "Project files have been combined into $output_file"
