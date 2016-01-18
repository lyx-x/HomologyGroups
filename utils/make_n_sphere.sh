#!/usr/bin/env bash

#Ex: bash make_n_sphere.sh 18

time=$1

for (( c=0; c<=$time; c++ ))
do
   python make_n_sphere.py $c
done