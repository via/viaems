import argparse
import sys

from viaems.connector import ExecConnector
from betterproto2 import Casing
from viaems_proto.viaems import console

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

    if args.get_config:
        config = conn.getconfig()
        print(config.to_json(indent=True, casing=Casing.SNAKE))

    if args.set_config:
        config = console.Configuration.from_json(sys.stdin.read())
        print(config)
        conn.setconfig(config)

        newconfig = conn.getconfig()
        print(newconfig.to_json(indent=True, casing=Casing.SNAKE))

    conn.kill()

