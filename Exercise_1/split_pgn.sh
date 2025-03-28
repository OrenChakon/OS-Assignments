#!/bin/bash
if [ $# -ne 2 ]; then
    echo "Usage: $0 <source_pgn_file> <destination_directory>"
    exit 1
fi

source_pgn_file=$1
destination_directory=$2

if [ ! -f "$source_pgn_file" ]; then
    echo "Error: File '$input_file' does not exist."
    exit 1
fi

if [ ! -d "$destination_directory" ]; then
    echo "Created directory '$dest_dir'."
    mkdir -p "$destination_directory"
fi

# Split the PGN file into separate files for each game
basename="${source_pgn_file##*/}"  # Removes everything before the last '/'
file_name="${basename%.*}"       # Removes everything after the last '.'
game_number=0
destination_file="$destination_directory/$file_name_$game_number.pgn"

while IFS= read -r line; do
    if [[ "$line" == *"[Event "* ]]; then
        game_number=$((game_number + 1))
        destination_file="$destination_directory/${file_name}_$game_number.pgn"
    fi
    echo "$line" >> "$destination_file"
done < "$source_pgn_file"

