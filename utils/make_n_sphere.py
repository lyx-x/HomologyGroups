#!/usr/bin/python

import sys
import itertools

number = int(sys.argv[1])

print 'Dim:', number

output = open("../filtrations/" + str(number) + "-sphere.txt", 'w');

for i in range(1, number + 2):
        for comb in itertools.combinations(range(1, number + 3), i):
                output.write("{} {} {}\n".format(i, i - 1, ' '.join(map(str, comb))))


