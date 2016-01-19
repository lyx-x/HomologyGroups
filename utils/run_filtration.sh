#!/usr/bin/env bash

#Ex: bash run_filtration.sh mobius-strip

Name=$1

../Release/filtration ../filtrations/$Name.txt ../intervals/$Name.txt ../log/"$Name"_