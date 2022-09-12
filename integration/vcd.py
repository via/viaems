vcd_sig_ids = [chr(x) for x in range(33, 126)]
def dump_vcd_signal(f, sig, val):
  if type(val) is float:
    f.write(f"r{val} {vcd_sig_ids[sig]}\n")
  else:
    f.write(f"{val}{vcd_sig_ids[sig]}\n")

def dump_vcd_line(f, time, trigger, outputs, sync, rpm):
  f.write("#{0}\n".format(time))
  dump_vcd_signal(f, 0, 1 if trigger == 0 else 0)
  dump_vcd_signal(f, 1, 1 if trigger == 1 else 0)
  dump_vcd_signal(f, 2, 1 if sync else 0)
  for i in range(16):
    dump_vcd_signal(f, i + 3, outputs[i])
  dump_vcd_signal(f, 20, float(rpm))

def dump_log_to_vcd(log, f):
  outputs = [0 for x in range(16)]
  f.write("$timescale 250ns $end\n")
  f.write("$scope module viaems $end\n")

  sig_names = ["trigger1", "trigger2", "sync"]
  for i in range(1, 16):
    sig_names.append(f"output_{i}")

  for i, name in enumerate(sig_names):
    f.write(f"$var wire 1 {vcd_sig_ids[i]} {name} $end\n")
  f.write(f"$var real 64 {vcd_sig_ids[20]} rpm $end\n")


  f.write("$upscope $end\n")
  f.write("$enddefinitions $end\n")
  f.write("$dumpvars\n")

  fields = []
  feed = {}

  def is_synced():
    if "sync" not in feed:
      return False
    return feed["sync"] == 1

  def rpm():
    if "rpm" not in feed:
      return 0
    return feed["rpm"]

  for e in log:
    time = e["time"]
    if "event" in e and e["event"]["type"] == "trigger":
      dump_vcd_line(f, time, e["event"]["pin"], outputs, is_synced(), rpm())
      dump_vcd_line(f, time + 1, None, outputs, is_synced(), rpm())
    if "event" in e and e["event"]["type"] == "output":
      outbits = e["event"]["outputs"]
      outputs = [1 if outbits & (1 << x) > 0 else 0 
          for x in range(16)]
      dump_vcd_line(f, time, None, outputs, is_synced(), rpm())
    if e["type"] == "description":
      fields = e["keys"]
    if e["type"] == "feed":
      values = e["values"]
      feed = dict(zip(fields, values))
      dump_vcd_line(f, time, None, outputs, is_synced(), rpm())
      

