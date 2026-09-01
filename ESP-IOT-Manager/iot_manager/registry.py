from __future__ import annotations

import json
import threading
import time
import urllib.request
from pathlib import Path
from typing import Any


class DeviceRegistry:
    """Thread-safe device inventory keyed by the board's stable MAC-based ID."""

    def __init__(self, data_file: Path, offline_after: float = 15.0) -> None:
        self.data_file = data_file
        self.offline_after = offline_after
        self._lock = threading.RLock()
        self._devices: dict[str, dict[str, Any]] = {}
        self._service_ids: dict[str, str] = {}
        self._aliases = self._load_aliases()

    def _load_aliases(self) -> dict[str, str]:
        try:
            data = json.loads(self.data_file.read_text(encoding="utf-8"))
            aliases = data.get("aliases", {})
            return aliases if isinstance(aliases, dict) else {}
        except (FileNotFoundError, json.JSONDecodeError, OSError):
            return {}

    def _save_aliases(self) -> None:
        self.data_file.parent.mkdir(parents=True, exist_ok=True)
        temp_file = self.data_file.with_suffix(".tmp")
        temp_file.write_text(
            json.dumps({"aliases": self._aliases}, indent=2) + "\n",
            encoding="utf-8",
        )
        temp_file.replace(self.data_file)

    def observe(
        self,
        service_name: str,
        address: str,
        port: int,
        properties: dict[str, str],
    ) -> None:
        device_id = properties.get("id") or f"service:{service_name}"
        now = time.time()
        with self._lock:
            previous = self._devices.get(device_id, {})
            previous.update(
                {
                    "id": device_id,
                    "service_name": service_name,
                    "address": address,
                    "port": port,
                    "type": properties.get("type", previous.get("type", "unknown")),
                    "model": properties.get("model", previous.get("model", "Unknown ESP device")),
                    "api": properties.get("api", previous.get("api", "1")),
                    "last_discovered": now,
                }
            )
            self._devices[device_id] = previous
            self._service_ids[service_name] = device_id

    def remove_service(self, service_name: str) -> None:
        with self._lock:
            device_id = self._service_ids.pop(service_name, None)
            if device_id and device_id in self._devices:
                self._devices[device_id]["last_discovered"] = 0

    def set_alias(self, device_id: str, alias: str) -> bool:
        alias = alias.strip()[:80]
        with self._lock:
            if device_id not in self._devices and device_id not in self._aliases:
                return False
            if alias:
                self._aliases[device_id] = alias
            else:
                self._aliases.pop(device_id, None)
            self._save_aliases()
            return True

    @staticmethod
    def _get_json(url: str, timeout: float = 1.5) -> dict[str, Any]:
        request = urllib.request.Request(
            url,
            headers={"Accept": "application/json", "User-Agent": "ESP-IOT-Manager/1"},
        )
        with urllib.request.urlopen(request, timeout=timeout) as response:
            if response.status != 200:
                raise OSError(f"HTTP {response.status}")
            payload = json.loads(response.read().decode("utf-8"))
            if not isinstance(payload, dict):
                raise ValueError("Expected a JSON object")
            return payload

    def poll(self) -> None:
        with self._lock:
            targets = [
                (device_id, item["address"], item["port"])
                for device_id, item in self._devices.items()
            ]

        for original_id, address, port in targets:
            base_url = f"http://{address}:{port}"
            try:
                metadata = self._get_json(base_url + "/api/device")
                status_path = metadata.get("status", "/api/status")
                status = self._get_json(base_url + status_path)
            except (OSError, ValueError, json.JSONDecodeError):
                continue

            stable_id = str(metadata.get("id") or original_id)
            now = time.time()
            with self._lock:
                device = self._devices.pop(original_id, {})
                device.update(metadata)
                device.update(
                    {
                        "id": stable_id,
                        "address": address,
                        "port": port,
                        "status_data": status,
                        "last_seen": now,
                    }
                )
                self._devices[stable_id] = device
                service_name = device.get("service_name")
                if service_name:
                    self._service_ids[service_name] = stable_id

    def snapshot(self) -> list[dict[str, Any]]:
        now = time.time()
        result: list[dict[str, Any]] = []
        with self._lock:
            for device_id, stored in self._devices.items():
                device = dict(stored)
                last_seen = float(device.get("last_seen", 0))
                device["online"] = bool(last_seen and now - last_seen <= self.offline_after)
                device["alias"] = self._aliases.get(device_id, "")
                device["display_name"] = (
                    device["alias"]
                    or device.get("name")
                    or device.get("model")
                    or device_id
                )
                device["url"] = f"http://{device['address']}:{device['port']}{device.get('ui', '/')}"
                result.append(device)

        return sorted(
            result,
            key=lambda item: (not item["online"], item["display_name"].lower()),
        )
