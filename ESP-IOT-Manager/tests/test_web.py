import json
import tempfile
import threading
import unittest
import urllib.request
from pathlib import Path
from unittest.mock import patch
from urllib.parse import quote

from iot_manager.registry import DeviceRegistry
from iot_manager.web import DashboardServer


class DashboardControlApiTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        root = Path(self.temp_dir.name)
        self.registry = DeviceRegistry(root / "devices.json")
        self.server = DashboardServer(("127.0.0.1", 0), self.registry, root)
        self.thread = threading.Thread(target=self.server.serve_forever)
        self.thread.start()

    def tearDown(self):
        self.server.shutdown()
        self.thread.join()
        self.server.server_close()
        self.temp_dir.cleanup()

    def test_control_route_passes_only_device_control_and_value(self):
        device_id = "esp8266-aabbccddeeff"
        control_id = "powerJack"
        path = (
            f"/api/devices/{quote(device_id, safe='')}/controls/"
            f"{quote(control_id, safe='')}"
        )
        request = urllib.request.Request(
            f"http://127.0.0.1:{self.server.server_port}{path}",
            data=json.dumps({"value": True}).encode("utf-8"),
            headers={"Content-Type": "application/json"},
            method="POST",
        )

        with patch.object(
            self.registry,
            "apply_control",
            return_value={"deviceStatus": {"powerJack": True}},
        ) as apply_control:
            with urllib.request.urlopen(request) as response:
                payload = json.load(response)

        apply_control.assert_called_once_with(device_id, control_id, True)
        self.assertTrue(payload["ok"])
        self.assertTrue(payload["deviceStatus"]["powerJack"])


if __name__ == "__main__":
    unittest.main()
