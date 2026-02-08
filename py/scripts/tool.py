import argparse
import sys

from viaems.connector import ExecConnector
from viaems_proto import console_pb2 as console
from google.protobuf import json_format

def to_json(m):
    j = json_format.MessageToJson(m, 
                                  always_print_fields_with_no_presence=True,
                                  preserving_proto_field_name=True,
                                  indent=2)
    return j

def from_json(j, m):
    m = json_format.Parse(j, m)
    return m

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="viaems-tool.py",
        description="Helper tool for interfacing with a viaems target"
    )
    parser.add_argument(
        "--hosted",
        action="store_true",
        default=False,
        help="Connect to the hosted target via obj/hosted/viaems",
    )

    parser.add_argument(
        "--proxy",
        action="store_true",
        default=False,
        help="Connect to a real target via obj/hosted/proxy",
    )

    parser.add_argument(
        "--exec",
        action="store",
        help="Connect via an arbitrary process"
    )

    parser.add_argument( 
        "--get-config",
        action="store_true",
        default=False,
        help="Fetch a configuration from the target and write to stdout",
    )

    parser.add_argument( 
        "--fwinfo",
        action="store_true",
        default=False,
        help="Fetches firmware info from target"
    )

    parser.add_argument( 
        "--set-config",
        action="store_true",
        default=False,
        help="Read a config from stdin and issue a reconfiguration to the target"
    )

    parser.add_argument( 
        "--bootloader",
        action="store_true",
        default=False,
        help="Issue a reset-to-bootloader command"
    )

    parser.add_argument( 
        "--flash-config",
        action="store_true",
        default=False,
        help="Issue a command to flash running config to persistent storage"
    )

    parser.add_argument( 
        "--pause",
        action="store_true",
        default=False,
        help="Wait for input after starting to allow attaching debugger"
    )


    args = parser.parse_args()
    exe = None
    if args.hosted:
        exe = "obj/hosted/viaems"
    elif args.proxy:
        exe = "obj/hosted/proxy"
    elif args.exec is not None:
        exe = args.exec

    if exe is None:
        print("No target specified!", file=sys.stdout)
        sys.exit(1)

    
    conn = ExecConnector(binary=exe)
    conn.start()

    if args.pause:
        input(f"Waiting for input (pid {conn.process.pid})...")

    if args.get_config:
        config = conn.getconfig()
        j = to_json(config)
        print(j)

    if args.set_config:
        config = from_json(sys.stdin.read(), console.Configuration())
        conn.setconfig(config)

        newconfig = conn.getconfig()
        print(to_json(newconfig))

    if args.fwinfo:
        fwinfo = console.Request()
        fwinfo.fwinfo.SetInParent()
        resp = conn.request(fwinfo)
        print(to_json(resp))

    if args.bootloader:
        bootloader = console.Message()
        bootloader.resettobootloader.SetInParent()
        conn.send(bootloader)

    conn.kill()

