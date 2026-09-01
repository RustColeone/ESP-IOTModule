# ESP-IOT Web Dashboard

This directory is a static, GitHub Pages-compatible companion to the automatic
local manager. It connects from the user's browser directly to known ESP-IOT
devices on the same LAN; it does not proxy traffic through GitHub.

## Publish with GitHub Pages

1. Push this repository to GitHub.
2. Open **Settings → Pages** in the repository.
3. Under **Build and deployment**, choose **Deploy from a branch**.
4. Select the target branch and the `/docs` folder, then save.
5. Open the HTTPS URL provided by GitHub.

GitHub Pages must use HTTPS so supporting browsers can request Local Network
Access permission.

## Add a device

1. Flash the updated firmware in this repository.
2. Put the browser and device on the same LAN/VLAN.
3. Enter the hostname printed at boot, such as
   `iot-switch-84f3eba1b2c3.local`, or enter the device IP address.
4. Allow local-network access when the browser prompts.

The association and friendly aliases are stored in browser `localStorage` and
are never uploaded. A URL can prefill the add-device field using a `device`
query parameter:

```text
https://rustcoleone.github.io/ESP-IOTModule/?device=iot-switch-84f3eba1b2c3.local
```

That URL can be encoded into a QR label without embedding credentials.

## Security boundary

Firmware enables cross-origin access only for the read-only `/api/device` and
`/api/status` endpoints. The static dashboard cannot invoke power or fan
mutation APIs. Its **Open controls** link navigates to the device's own local UI.

## Browser support

Current Chrome or Edge is recommended. Ordinary web pages cannot browse mDNS,
so every device must be added once. The local Python manager remains the option
for automatic `_esp-iot._tcp.local` discovery.
