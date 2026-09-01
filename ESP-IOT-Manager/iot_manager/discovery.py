from __future__ import annotations

import ipaddress

from zeroconf import ServiceBrowser, ServiceStateChange, Zeroconf

from .registry import DeviceRegistry


SERVICE_TYPE = "_esp-iot._tcp.local."


def _decode_properties(properties: dict[bytes, bytes | None]) -> dict[str, str]:
    decoded: dict[str, str] = {}
    for key, value in properties.items():
        decoded[key.decode("utf-8", "replace")] = (
            value.decode("utf-8", "replace") if value is not None else ""
        )
    return decoded


class DeviceDiscovery:
    def __init__(self, registry: DeviceRegistry) -> None:
        self.registry = registry
        self.zeroconf = Zeroconf()
        self.browser = ServiceBrowser(
            self.zeroconf,
            SERVICE_TYPE,
            handlers=[self._on_service_state_change],
        )

    def _on_service_state_change(
        self,
        zeroconf: Zeroconf,
        service_type: str,
        name: str,
        state_change: ServiceStateChange,
    ) -> None:
        if state_change is ServiceStateChange.Removed:
            self.registry.remove_service(name)
            return

        info = zeroconf.get_service_info(service_type, name, timeout=2000)
        if info is None:
            return

        address = next(
            (
                str(ipaddress.ip_address(raw))
                for raw in info.addresses
                if len(raw) == 4
            ),
            None,
        )
        if address:
            self.registry.observe(
                name,
                address,
                info.port,
                _decode_properties(info.properties),
            )

    def close(self) -> None:
        self.browser.cancel()
        self.zeroconf.close()
