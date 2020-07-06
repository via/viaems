#!/usr/bin/env python
import sys

cur_outputs = [0 for i in range(8)]

#output_convert = [8, 9, 10, 0, 1, 2]
output_convert = [0, 1, 2, 8, 9, 10]

for line in sys.stdin:
    changed = False
    if line.startswith(";") or line.startswith("microseconds"):
        continue
    time = int(line.split(',')[0])# / (1000)
    outputs = [int(x) for x in line.split(',')[1:]]

    if cur_outputs[6] == 0 and outputs[6] == 1:
        print("# TRIGGER0 {}".format(time))
    if cur_outputs[7] == 0 and outputs[7] == 1:
        print("# TRIGGER1 {}".format(time))

    vals = 0
    for o in range(6):
        vals = vals | (outputs[o] << output_convert[o])
    print("# OUTPUTS {} {}".format(time, hex(vals)[2:]))


    cur_outputs = outputs
    time += 1
