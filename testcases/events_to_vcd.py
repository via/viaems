#!/usr/bin/env python
import sys
import re

def parse_event(ev):
  m = re.match(r"(\d+) trigger1\n", ev)
  if m is not None:
  
    return {"type": "trigger1", 
            "time": int(m.group(1))}
  m = re.match(r"(\d+) trigger2\n", ev)
  if m is not None:
  
    return {"type": "trigger2", 
            "time": int(m.group(1))}

  m = re.match(r"(\d+) output (\d+) (\d+) ([\d\.]+) (\d+)\n", ev)
  if m is not None:
    return {"type": "output",
            "time": int(m.group(1)),
            "output": int(m.group(2)),
            "value": int(m.group(3)),
            "advance": float(m.group(4)),
            "dwell": int(m.group(5))}

def dumpline(time, trigger, outputs):
    print "#{0}".format(time)
    print "1#" if trigger == 1 else "0#"
    print "1$" if trigger == 2 else "0$"
    print "{0}%".format(outputs[0])
    print "{0}&".format(outputs[1])
    print "{0}'".format(outputs[2])
    print "{0}(".format(outputs[3])
    print "{0})".format(outputs[4])
    print "{0}*".format(outputs[5])
    print "{0}+".format(outputs[6])
    print "{0},".format(outputs[7])

print "$timescale 1ns $end"
print "$scope module tfi $end"
print "$var wire 1 # trigger1 $end"
print "$var wire 1 $ trigger2 $end"
print "$var wire 1 % output_1 $end"
print "$var wire 1 & output_2 $end"
print "$var wire 1 ' output_3 $end"
print "$var wire 1 ( output_4 $end"
print "$var wire 1 ) output_5 $end"
print "$var wire 1 * output_6 $end"
print "$var wire 1 + output_7 $end"
print "$var wire 1 , output_8 $end"
print "$upscope $end"
print "$enddefinitions $end"
print "$dumpvars"

outputs = [0 for x in range(8)]
time = None


for line in sys.stdin.readlines():
    ev = parse_event(line)
    if time is None:
        time = ev['time']
    if ev['type'] == "trigger1":
        dumpline(ev['time'] - time, 1, outputs)
        dumpline(1 + ev['time'] - time, 0, outputs)
    if ev['type'] == "trigger2":
        dumpline(ev['time'] - time, 2, outputs)
        dumpline(1 + ev['time'] - time, 0, outputs)
    if ev['type'] == "output":
        outputs[ev['output']] = ev['value']
        dumpline(ev['time'] - time, 0, outputs)
         
