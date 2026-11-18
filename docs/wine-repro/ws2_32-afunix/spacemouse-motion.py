#!/usr/bin/env python3
# Original AI-assisted (Codex), LGPL-2.1-or-later.
"""Send sample spacenavd motion to an Inventor test through a private socket."""
import argparse
import os
from pathlib import Path
import select
import socket
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory
import time

FRAME = struct.Struct("<8i")


def wait_for_inventor(listener, launcher):
    listener.settimeout(0.25)
    while launcher.poll() is None:
        try:
            return listener.accept()[0]
        except TimeoutError:
            pass
    return None


def send_motion(connection):
    connection.settimeout(3)
    neutral = FRAME.pack(0, 0, 0, 0, 0, 0, 0, 30)
    motions = (
        FRAME.pack(0, 145, 0, 0, 0, 0, 0, 30),
        FRAME.pack(0, 0, 0, 120, 0, 0, 0, 30),
        FRAME.pack(0, 0, 0, 0, 0, 200, 0, 30),
    )
    for motion in motions:
        for _ in range(24):
            connection.sendall(motion)
            time.sleep(0.03)
        connection.sendall(neutral)
        time.sleep(0.6)
    print("Synthetic AF_UNIX motion sent (72 motion frames)", flush=True)


def run_trial(listener, launcher):
    connection = wait_for_inventor(listener, launcher)
    if connection is None:
        print("Inventor exited before connecting to the synthetic socket", flush=True)
        return 1
    with connection:
        print("Synthetic AF_UNIX peer connected.", flush=True)
        print("Open a test part in Inventor, then press Enter here. "
              "Return focus to Inventor within five seconds.", flush=True)
        while launcher.poll() is None:
            if select.select([sys.stdin], [], [], 0.25)[0]:
                break
        if launcher.poll() is not None or not sys.stdin.readline():
            return 1
        time.sleep(5)
        try:
            send_motion(connection)
        except OSError as error:
            print(f"Synthetic socket send failed (errno {error.errno})", flush=True)
            return 1
        print("Close Inventor to finish the trial.", flush=True)
        return launcher.wait()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="new file for launcher output")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="command to prepare and start Inventor")
    args = parser.parse_args()
    if not args.command:
        parser.error("provide a command to start Inventor")
    with TemporaryDirectory(prefix="winum-spnav-", dir="/tmp") as temp:
        endpoint = str(Path(temp) / "motion.sock")
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as listener, args.log.open("xb") as log:
            listener.bind(endpoint)
            listener.listen(1)
            env = dict(os.environ, SIAPPDLL_TRANSPORT="afunix", SIAPPDLL_SOCKET=endpoint)
            print("Starting Inventor. Waiting for the synthetic socket connection.", flush=True)
            with subprocess.Popen(args.command, env=env, stdout=log, stderr=subprocess.STDOUT) as launcher:
                return run_trial(listener, launcher)


if __name__ == "__main__":
    raise SystemExit(main())
