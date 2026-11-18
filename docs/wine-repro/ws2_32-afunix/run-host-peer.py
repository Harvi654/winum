#!/usr/bin/env python3
"""Test both PE clients against a local Linux AF_UNIX listener."""
# Original AI-assisted (Codex), LGPL-2.1-or-later.
import argparse
import os
from pathlib import Path
import socket
import subprocess
import tempfile


def exchange(listener, wine, client, endpoint, env, extended=False):
    args = [wine, client, endpoint]
    if extended:
        args.append("--connectex")
    exchanged = False
    with subprocess.Popen(
        args, env=env, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True
    ) as process:
        try:
            with listener.accept()[0] as connection:
                connection.settimeout(5)
                request = b""
                if not extended:
                    while len(request) < 4:
                        chunk = connection.recv(4 - len(request))
                        if not chunk:
                            break
                        request += chunk
                if extended or request == b"PING":
                    connection.sendall(b"PONG")
                    exchanged = True
        except OSError:
            pass

        try:
            output, _ = process.communicate(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            output, _ = process.communicate()
        print(f"{client.name}: {output.strip()}")
        expected = "host_connectex_peer_closed=ok" if extended else "host_exchange=ok"
        return exchanged and expected in output.splitlines() and process.returncode == 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wine", type=Path, help="path to the candidate's bin/wine")
    parser.add_argument("bundle", type=Path, help="directory containing both host-peer executables")
    parser.add_argument("--measure", action="store_true",
                        help="also run both Windows MEASURE executables under Wine")
    parser.add_argument("--connectex", action="store_true",
                        help="also test deferred ConnectEx against a peer that sends and closes immediately")
    args = parser.parse_args()
    wine = args.wine.absolute()
    wineserver = wine.with_name("wineserver")
    clients = [args.bundle.resolve() / f"host-peer-{arch}.exe" for arch in ("x86_64", "i686")]
    measures = [args.bundle.resolve() / f"afunix-v4-{arch}.exe" for arch in ("x86_64", "i686")]
    for path in [wine, wineserver, *clients, *(measures if args.measure else ())]:
        if not path.is_file():
            parser.error(f"missing {path.name}")

    with tempfile.TemporaryDirectory(prefix="winum-afunix-", dir="/tmp") as temp:
        root = Path(temp)
        env = dict(
            os.environ, WINEPREFIX=str(root / "prefix"), WINELOADER=str(wine),
            WINESERVER=str(wineserver), WINEDEBUG="-all", WINEDLLOVERRIDES="mscoree,mshtml="
        )
        env.pop("WINEARCH", None)
        try:
            subprocess.run([wine, "wineboot", "-i"], env=env, check=True, timeout=90,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            dosdevices = root / "prefix" / "dosdevices"
            if not dosdevices.is_dir():
                raise RuntimeError("wineboot did not create the drive mapping directory")
            (dosdevices / "z:").unlink(missing_ok=True)
            endpoint = root / "host.sock"
            outcomes = []
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as listener:
                listener.bind(str(endpoint))
                listener.listen()
                listener.settimeout(8)
                for client in clients:
                    outcomes.append(exchange(listener, wine, client, endpoint, env))
                    if args.connectex:
                        outcomes.append(exchange(listener, wine, client, endpoint, env, extended=True))
            if os.path.lexists(dosdevices / "z:"):
                raise RuntimeError("Wine recreated the Z drive mapping during the host test")
            if args.measure:
                for exe in measures:
                    sample = root / "prefix" / "drive_c" / f"sample-{exe.stem}"
                    sample.mkdir()
                    result = subprocess.run([wine, exe, "C:\\" + sample.name], env=env, check=False,
                                            timeout=45, capture_output=True, text=True)
                    print(f"{exe.name} exit={result.returncode}")
                    print(result.stdout.strip())
            print("host_peer_both_architectures=" + ("ok" if all(outcomes) else "failed"))
            return 0 if all(outcomes) else 1
        finally:
            subprocess.run([wineserver, "-k"], env=env, check=False, timeout=10,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            subprocess.run([wineserver, "-w"], env=env, check=False, timeout=10,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


if __name__ == "__main__":
    raise SystemExit(main())
