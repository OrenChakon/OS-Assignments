#!/bin/bash


calculate_board_state() {
    board_state=$1
    move=$2
    move_start=${move:0:2}
    move_end=${move:2:2}
    move_start_letter=${move_start:0:1}
    move_start_number=${move_start:1:1}
    move_end_letter=${move_end:0:1}
    move_end_number=${move_end:1:1}
    move_start_location=$(move_to_board_state_location $move_start)
    move_end_location=$(move_to_board_state_location $move_end)
    piece=${board_state:move_start_location:1}
    end_piece=${board_state:move_end_location:1}

        #check for en passant
    if [ "$piece" = "p" ] && [ $move_start_number -eq 4 ] && [ "$move_end_letter" != "$move_start_letter" ] && [ "$end_piece" = "." ]; then
        board_state=$(move_piece $board_state ($move_end_letter)($move_end_number)($move_end_letter)($move_end_number+1))
    elif [ "$piece" = "P" ] && [ $move_start_number -eq 4 ] && [ "$move_end_letter" != "$move_start_letter" ] && [ "$end_piece" = "." ]; then
        board_state=$(move_piece $board_state ($move_end_letter)($move_end_number)($move_end_letter)($move_end_number-1))
    fi

    board_state=$(move_piece $board_state $move)

    # check for castling
    if [[ "$piece" = "K" ]] && [[ "$move_start_letter" = "e" ]] && [[ "$move_end_letter" = "c" ]]; then
        board_state=$(move_piece $board_state "a1d1")
    elif [[ "$piece" = "K" ]] && [[ "$move_start_letter" = "e" ]] && [[ "$move_end_letter" = "g" ]]; then
        board_state=$(move_piece $board_state "h1f1")
    elif [[ "$piece" = "k" ]] && [[ "$move_start_letter" = "e" ]] && [[ "$move_end_letter" = "c" ]]; then
        board_state=$(move_piece $board_state "a8d8")
    elif [[ "$piece" = "k" ]] && [[ "$move_start_letter" = "e" ]] && [[ "$move_end_letter" = "g" ]]; then
        board_state=$(move_piece $board_state "h8f8")
    fi

    #promotation 
    if [[ "$piece" = "P" ]] && (( move_end_number == 8 )) || [[ "$piece" = "p" ]] && (( move_end_number == 1 )); then
        promote_piece=${move:4:1}
        board_state=${board_state:0:move_end_location}$promote_piece${board_state:move_end_location+1}
    fi
    echo $board_state
}

move_piece() {
    board_state=$1
    move=$2
    move_start=${move:0:2}
    move_end=${move:2:2}
    move_start_location=$(move_to_board_state_location $move_start)
    move_end_location=$(move_to_board_state_location $move_end)
    piece=${board_state:move_start_location:1}
    board_state=${board_state:0:move_start_location}"."${board_state:move_start_location+1}
    board_state=${board_state:0:move_end_location}$piece${board_state:move_end_location+1}
    echo $board_state
}

move_to_board_state_location() {
    move=$1
    move_letter=${move:0:1}
    move_number=${move:1:1}
    letter_to_number=$(( $(printf "%d" "'$move_letter") - 97 ))
    number_to_location=$(((8 - ${move_number}) * 8 + $letter_to_number))
    echo $number_to_location
}

display_board() {
    board_state_array=$1
    move=$2
    echo "Move $move/${#board_state_array[@]}"
    echo "  a b c d e f g h"
    for (( i=0; i<64; i++ )); do
        if (( i % 8 == 0 )); then
            echo -n "$((8-i/8))"
        fi
        echo -n " ${board_state_array[$move]:i:1} "
        if (( i % 8 == 7 )); then
            echo
        fi
    done
    echo "  a b c d e f g h"


}



if [ $# -ne 1 ]; then
    echo "Usage: $0 <pgn_file>"
    exit 1
fi

pgn_file=$1

if [ ! -f "$pgn_file" ]; then
    echo "Error: File '$pgn_file' does not exist."
    exit 1
fi
moves=""
echo "Metadata from PGN file:"
while IFS= read -r line; do
    if [[ "$line" == *"["* ]]; then
        echo "$line"
    else
        moves+="$line"
        moves+=" "
    fi
done < "$pgn_file"
echo
moves=$(python3 parse_moves.py "$moves")
moves_arr=($moves)
echo $moves
move=0
board_state_array=("rnbqkbnrpppppppp................................PPPPPPPPRNBQKBNR")
while [ $move -lt ${#moves_arr[@]} ]; do

    board_state_array+=($(calculate_board_state ${board_state_array[$move]} ${moves_arr[$move]}))
    move=$(($move+1))
done
move=0
display_board "${board_state_array}" $move
echo -n "press 'd' to move forward, 'a' to move back, 'w' to go to the start, 's' to go to the end, 'q' to quit: "
read -n 1 -s key
echo $key
while true; do
    if [ "$key" = "a" ]; then
        move=$(($move-1))
        if [ $move -lt 0 ]; then
            move=0
        fi
    elif [ "$key" = "d" ]; then
        move=$(($move+1))
        if [ $move -ge ${#board_state_array[@]} ]; then
            move=$((${#board_state_array[@]} - 1))
        fi
    elif [ "$key" = "w" ]; then
        move=0
    elif [ "$key" = "s" ]; then
        move=$((${#board_state_array[@]} - 1))
    elif [ "$key" = "q" ]; then
        exit 0
    fi
    display_board "${board_state_array}" $move
    echo -n "press 'd' to move forward, 'a' to move back, 'w' to go to the start, 's' to go to the end, 'q' to quit: "
    read -n 1 -s key
    echo $key
done
