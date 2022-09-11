vcd_sig_ids = [chr(x) for x in range(33, 126)]
def dump_vcd_signal(f, sig, val):
  f.write(f"{val}{vcd_sig_ids[sig]}\n")

def dump_vcd_line(f, time, trigger, outputs):
  f.write("#{0}\n".format(time))
  dump_vcd_signal(f, 0, 1 if trigger == 0 else 0)
  dump_vcd_signal(f, 1, 1 if trigger == 1 else 0)
  for i in range(16):
    dump_vcd_signal(f, i + 2, outputs[i])

def dump_log_to_vcd(log, f):
  outputs = [0 for x in range(16)]
  f.write("$timescale 250ns $end\n")
  f.write("$scope module viaems $end\n")

  sig_names = ["trigger1", "trigger2"]
  for i in range(1, 16):
    sig_names.append(f"output_{i}")

  for i, name in enumerate(sig_names):
    f.write(f"$var wire 1 {vcd_sig_ids[i]} {name} $end\n")

  f.write("$upscope $end\n")
  f.write("$enddefinitions $end\n")
  f.write("$dumpvars\n")

  for e in log:
    if e["type"] != "event":
      continue
    time = e["time"]
    if e["event"]["type"] == "trigger":
      dump_vcd_line(f, time, e["event"]["pin"], outputs)
      dump_vcd_line(f, time + 1, None, outputs)
    if e["event"]["type"] == "output":
      outbits = e["event"]["outputs"]
      outputs = [1 if outbits & (1 << x) > 0 else 0 
          for x in range(16)]
      dump_vcd_line(f, time, None, outputs)

