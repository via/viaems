#!/usr/bin/env python
import sys
import re

def test_dwell(events):
    start = None
    for ev in events:
        if ev['type'] == "output":
            if ev['value'] == '1' and start is None:
                print "error double low: ", ev
            if ev['value'] == '0' and start is not None:
                print "error double high: ", ev
            if ev['value'] == '1' and start is not None:
                time = int(ev['time']) - int(start)
                start = None
                delta = abs(time / 1000 - int(ev['dwell']))
                print "dwell delta ", delta
                if delta > 50:
                    print "error dwell delta ", delta
            if ev['value'] == '0' and start is None:
                start = int(ev['time'])
   

def parse_event(ev):
  m = re.match(r"(\d+) trigger1\n", ev)
  if m is not None:
  
    return {"type": "trigger1", 
            "time": m.group(1)}

  m = re.match(r"(\d+) output (\d+) (\d+) (\d+) ([\d\.]+) (\d+)\n", ev)
  if m is not None:
    return {"type": "output",
            "time": m.group(1),
            "output": m.group(2),
            "value": m.group(3),
            "rpm": m.group(4),
            "advance": m.group(5),
            "dwell": m.group(6)}
    

events = sys.stdin.readlines()
events = [parse_event(e) for e in events]

test_dwell(events)
