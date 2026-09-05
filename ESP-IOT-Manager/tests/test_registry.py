import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from iot_manager.registry import DeviceRegistry, InvalidControlValueError


class DeviceRegistryTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.registry = DeviceRegistry(Path(self.temp_dir.name) / "devices.json")
        self.registry.observe(
            "Switch._esp-iot._tcp.local.",
            "192.168.1.20",
            80,
            {"id": "esp8266-aabbccddeeff", "type": "power-switch"},
        )

    def tearDown(self):
        self.temp_dir.cleanup()

    def test_same_firmware_devices_remain_distinct(self):
        self.registry.observe(
            "Switch 2._esp-iot._tcp.local.",
            "192.168.1.21",
            80,
            {"id": "esp8266-112233445566", "type": "power-switch"},
        )
        self.assertEqual(2, len(self.registry.snapshot()))

    def test_ip_change_updates_same_device(self):
        self.registry.observe(
            "Switch._esp-iot._tcp.local.",
            "192.168.1.99",
            80,
            {"id": "esp8266-aabbccddeeff", "type": "power-switch"},
        )
        devices = self.registry.snapshot()
        self.assertEqual(1, len(devices))
        self.assertEqual("192.168.1.99", devices[0]["address"])

    def test_alias_is_persisted_by_stable_id(self):
        self.assertTrue(self.registry.set_alias("esp8266-aabbccddeeff", "Desk Switch"))
        restored = DeviceRegistry(Path(self.temp_dir.name) / "devices.json")
        restored.observe(
            "Switch._esp-iot._tcp.local.",
            "192.168.1.45",
            80,
            {"id": "esp8266-aabbccddeeff"},
        )
        self.assertEqual("Desk Switch", restored.snapshot()[0]["display_name"])

    @patch.object(DeviceRegistry, "_get_json")
    def test_successful_poll_marks_device_online(self, get_json):
        get_json.side_effect = [
            {"id": "esp8266-aabbccddeeff", "name": "Power Switch ddeeff"},
            {"powerJack": True},
        ]
        self.registry.poll()
        device = self.registry.snapshot()[0]
        self.assertTrue(device["online"])
        self.assertTrue(device["status_data"]["powerJack"])

    @patch.object(DeviceRegistry, "_get_json")
    def test_toggle_control_uses_advertised_endpoint(self, get_json):
        get_json.side_effect = [
            {
                "id": "esp8266-aabbccddeeff",
                "controls": [
                    {
                        "id": "powerJack",
                        "type": "toggle",
                        "endpoint": "/api/powerjack",
                        "valueField": "state",
                    }
                ],
            },
            {"powerJack": False},
        ]
        self.registry.poll()

        with patch.object(DeviceRegistry, "_post_json", return_value={"success": True}) as post:
            get_json.side_effect = None
            get_json.return_value = {"powerJack": True}
            result = self.registry.apply_control(
                "esp8266-aabbccddeeff",
                "powerJack",
                True,
            )

        post.assert_called_once_with(
            "http://192.168.1.20:80/api/powerjack",
            {"state": True},
        )
        self.assertTrue(result["deviceStatus"]["powerJack"])
        self.assertTrue(self.registry.snapshot()[0]["status_data"]["powerJack"])

    @patch.object(DeviceRegistry, "_get_json")
    def test_range_control_rejects_out_of_range_value(self, get_json):
        get_json.side_effect = [
            {
                "id": "esp8266-aabbccddeeff",
                "controls": [
                    {
                        "id": "fan0",
                        "type": "range",
                        "endpoint": "/api/fan/0",
                        "valueField": "speed",
                        "min": 0,
                        "max": 100,
                    }
                ],
            },
            {"fans": [{"speed": 0, "rpm": 0}]},
        ]
        self.registry.poll()

        with self.assertRaises(InvalidControlValueError):
            self.registry.apply_control("esp8266-aabbccddeeff", "fan0", 101)


if __name__ == "__main__":
    unittest.main()
