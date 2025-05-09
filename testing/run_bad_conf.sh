#!/bin/bash

DIR="../config/bad"
PROGRAM="../webserv"

if [ ! -f "$PROGRAM" ]; then
    echo "Error: Program '$PROGRAM' not found!"
    exit 1
fi

if [ ! -d "$DIR" ]; then
    echo "Error: Directory '$DIR' not found!"
    exit 1
fi

USE_VALGRIND=false
if [ "$1" == "mem" ]; then
    USE_VALGRIND=true
    VALGRIND_CMD="valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes"
    echo "Running with Valgrind..."
else
    VALGRIND_CMD=""
fi

for conf in "$DIR"/bad_*
do
    if [ -f "$conf" ]; then
        echo "Running with conf: $conf"
        if $USE_VALGRIND; then
            $VALGRIND_CMD "$PROGRAM" "$conf"
        else
            "$PROGRAM" "$conf"
        fi
        echo "-------------------------------------"
    else
        echo "No 'bad_' files found in $DIR"
        break
    fi
done
