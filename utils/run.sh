#!/usr/bin/env bash

# Run all filtrations registered in a file
#Ex: bash run.sh items.txt

File=$1

while read -r Line
do
    echo "Running $Line ..."
    bash run_filtration.sh $Line
done < "$File"