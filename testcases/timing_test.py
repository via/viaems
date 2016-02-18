#!/usr/bin/env python
import sys
import re
import numpy

def test_dwell_times(events):
    print '* Checking dwell time error (us)'
    starts = [None for x in range(24)]
    deltas = []
    for ev in events:
        if ev['type'] == "output":
            if ev['value'] == 1 and starts[ev['output']] is None:
                print " [E] double low: ", ev
            if ev['value'] == 0 and starts[ev['output']] is not None:
                print " [E] double high: ", ev
            if ev['value'] == 1 and starts[ev['output']] is not None:
                time = ev['time'] - starts[ev['output']]
                starts[ev['output']] = None
                delta = abs(time / 1000 - ev['dwell'])
                deltas.append(delta)
            if ev['value'] == 0 and starts[ev['output']] is None:
                starts[ev['output']] = ev['time']
    ndeltas = numpy.array(deltas)
    print " [I] min: {0} max: {1} mean: {2:.2} stddev: {3:.2}".format(
        numpy.amin(ndeltas), numpy.amax(ndeltas),
        numpy.mean(ndeltas), numpy.std(ndeltas))
    print
              
def advance(time, rpm):
    ticks_per_degree = (1000000000.0 / 6) / rpm
    return time / ticks_per_degree

def test_advances(events):
    print "* Checking timing advance error for sanity (degrees)"
    lasttrigger = None
    rpm = None
    errors = []
    for ev in events:
        if ev['type'] == "trigger1":
            if lasttrigger is not None:
                rpm = (1000000000.0 / 6) / ((ev['time'] - lasttrigger) / 90)
            lasttrigger = ev['time']
        
        if ev['type'] == "output" and ev['value'] == 1:
            # Ignition is firing, calculate angle past trigger
            if lasttrigger is not None and rpm is not None:
                adv = 45 - advance(ev['time'] - lasttrigger, rpm)
                errors.append(adv - ev['advance'])
                
    ndeltas = numpy.array(errors)
    print " [I] min: {0:.2} max: {1:.2} mean: {2:.2} stddev: {3:.2}".format(
        numpy.amin(ndeltas), numpy.amax(ndeltas),
        numpy.mean(ndeltas), numpy.std(ndeltas))
    print
            
   

def parse_event(ev):
  m = re.match(r"(\d+) trigger1\n", ev)
  if m is not None:
  
    return {"type": "trigger1", 
            "time": int(m.group(1))}

  m = re.match(r"(\d+) output (\d+) (\d+) ([\d\.]+) (\d+)\n", ev)
  if m is not None:
    return {"type": "output",
            "time": int(m.group(1)),
            "output": int(m.group(2)),
            "value": int(m.group(3)),
            "advance": float(m.group(4)),
            "dwell": int(m.group(5))}
    

events = sys.stdin.readlines()
events = [parse_event(e) for e in events]

test_dwell_times(events)
test_advances(events)
