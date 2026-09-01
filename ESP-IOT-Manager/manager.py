from __future__ import annotations

import argparse
import threading
from pathlib import Path

from iot_manager.discovery import DeviceDiscovery
from iot_manager.registry import DeviceRegistry
from iot_manager.web import DashboardServer


ROOT = Path(__file__).resolve().parent


def poll_forever(registry: DeviceRegistry, stop: threading.Event) -> None:
    while not stop.wait(2.0):
        registry.poll()


def main() -> None:
    parser = argparse.ArgumentParser(description="Discover and manage ESP-IOT devices")
    parser.add_argument("--host", default="127.0.0.1", help="dashboard bind address")
    parser.add_argument("--port", default=8080, type=int, help="dashboard port")
    args = parser.parse_args()

    registry = DeviceRegistry(ROOT / "data" / "devices.json")
    discovery = DeviceDiscovery(registry)
    stop = threading.Event()
    poller = threading.Thread(target=poll_forever, args=(registry, stop), daemon=True)
    poller.start()
    server = DashboardServer((args.host, args.port), registry, ROOT / "static")

    print(f"ESP-IOT Manager: http://{args.host}:{args.port}")
    print("Discovering _esp-iot._tcp.local devices (Ctrl+C to stop)")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping manager...")
    finally:
        stop.set()
        server.server_close()
        discovery.close()


if __name__ == "__main__":
    main()
