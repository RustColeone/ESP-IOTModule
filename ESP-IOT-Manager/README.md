# ESP-IOT Manager

A local web dashboard that automatically discovers the ESP devices in this
repository. Devices advertise `_esp-iot._tcp.local` over mDNS, so DHCP address
changes do not require any manager configuration.

For users who should not run a local manager, the companion
[static web dashboard](../docs/README.md) can be hosted on GitHub Pages. It
requires each device to be added once because ordinary web pages cannot browse
mDNS.

## Device identity

Every board derives a stable ID from its full 48-bit Wi-Fi MAC address. The ID,
not the IP address or firmware type, is the manager's registry key. Consequently:

- Multiple boards flashed with identical firmware appear as separate devices.
- A DHCP address change updates the existing card instead of creating a new one.
- Rebooting or reflashing the same board preserves its manager alias.
- Replacing the physical board creates a new device because its MAC is different.

Hostnames include the full compact MAC, for example
`iot-switch-84f3eba1b2c3.local`, eliminating collisions between boards. The
short six-digit suffix is used only in the default display name. Friendly
aliases are stored in `data/devices.json` by the manager and do not consume
device EEPROM/NVS.

## Requirements

- Python 3.10 or newer
- A computer on the same multicast-capable LAN/VLAN as the ESP devices
- Updated firmware from this repository flashed onto each device

## Run

From this directory in PowerShell:

```powershell
py -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
python manager.py
```

Open <http://127.0.0.1:8080>.

To make the dashboard reachable from another computer or phone on the LAN:

```powershell
python manager.py --host 0.0.0.0 --port 8080
```

Then open `http://MANAGER_COMPUTER_IP:8080`. The dashboard has no login, so do
not expose this port to the internet.

## Discovery contract

Firmware advertises this DNS-SD service:

```text
Service: _esp-iot._tcp.local
Port:    80
TXT:     id, type, model, api
```

It also exposes `GET /api/device` with common metadata. Device-specific live
state remains at `GET /api/status`.

## Tests

The registry tests use only Python's standard library:

```powershell
python -m unittest discover -s tests -v
```

## Network limitations

mDNS usually remains within one LAN/VLAN. If multicast is disabled or the
manager is on a different VLAN, discovery will not cross the router without an
mDNS reflector. Existing cards become offline after 15 seconds rather than
being silently deleted.
