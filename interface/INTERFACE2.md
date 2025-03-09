## WIP new protobuf based interface


Command/Response as protobuf messages. The platform api exposes a read and write
api that abstracts over the underlying physical transport.

For unreliable byte stream transports, such as a serial link, messages are
prepended with a header:
2 byte message length
4 byte CRC-32 (HDLC)
They are then COBS encoded and send with zero as a delimiter

