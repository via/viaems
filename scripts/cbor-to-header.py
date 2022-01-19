import sys

print("#ifndef CONF")
print("#define CONF")
print("")
print("#include <stdint.h>")
print("")
print("static const uint8_t cbor_configuration[] = {")

data = sys.stdin.buffer.read()

horiz_pos = 0
for byte in data:
  print(f"0x{byte:02x}, ", end="")
  horiz_pos += 1
  if horiz_pos > 20:
    horiz_pos = 0
    print()


print("};")
print("")
print("#endif")
