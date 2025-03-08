## WIP new protobuf based interface


Command/Response as protobuf messages.

Messages are broken into fragments. The intent is to provide a robust means of
using different transports, such as ethernet or a serial link.

These fragments are bounded size with a fragment header indicating how it can be
reassembled, all values little endian:

2 byte message number
2 byte fragment number
N byte payload (PDU)

For a byte oriented stream like a serial link, fragments are prepended with an
extra link header: 
2 byte fragment length (PDU + fragment header)
2 byte check (PDU + fragment header)

and then fragments are COBS-encoded using a 0 byte delimiter. Currently the max
PDU size is 240 bytes before header, 244 bytes including the fragment header.

For packet oriented links, one fragment is in PDU of the packet of the link.
