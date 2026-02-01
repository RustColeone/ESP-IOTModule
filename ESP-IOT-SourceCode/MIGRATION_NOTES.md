# ESP32-C6 Migration Notes

## Hardware Changes Summary

### Microcontroller
- **Previous**: ESP8266
- **New**: ESP32-C6

### Pin Mapping Changes

#### Button Inputs (with internal pullup)
- **Button 1**: GPIO4 → Toggle Power Jack
- **Button 2**: GPIO5 → Toggle USB Output
- **Button 3**: GPIO6 → Cycle PD Voltage (5V → 9V → 12V → 15V → 20V → 5V)
- **Button 4**: GPIO7 → Enable All Outputs

#### CH224K PD Control Pins
- **CFG1**: GPIO18 (SDIO_CLK)
- **CFG2**: GPIO19 (SDIO_DATA0)
- **CFG3**: GPIO20 (SDIO_DATA1)

#### Output Control Pins
- **Power Jack Enable**: GPIO11 (HIGH = enable, LOW = disable)
- **USB Output Enable**: GPIO10 (LOW = enable, HIGH = disable) [Inverted Logic]

#### ADC Voltage Sensing
- **VBUS ADC**: GPIO2 (monitors requested PD voltage)
- **VOUT ADC**: GPIO3 (monitors output jack voltage)

### CH224K PD Voltage Configuration

| Voltage | CFG1 | CFG2 | CFG3 | Binary |
|---------|------|------|------|--------|
| 5V      | HIGH | LOW  | LOW  | 1XX    |
| 9V      | LOW  | LOW  | LOW  | 000    |
| 12V     | LOW  | LOW  | HIGH | 001    |
| 15V     | LOW  | HIGH | HIGH | 011    |
| 20V     | LOW  | HIGH | LOW  | 010    |

## Code Changes

### Modified Files

1. **config.h**
   - Updated pin definitions for ESP32-C6
   - Added ADC configuration constants
   - Added VBUS/VOUT divider ratios
   - Split `powerState` into `powerJackState` and `usbOutputState`
   - Updated Config struct with new fields

2. **hardware.h/cpp**
   - Replaced `setPowerState()` with `setPowerJackState()` and `setUSBOutputState()`
   - Added `getVOutVoltage()` function
   - Updated `setPDVoltage()` to control all 3 CFG pins (including CFG1)
   - Added 5V PD voltage support
   - Updated button handlers for new functionality

3. **ESP-IOT-SourceCode.ino**
   - Updated for ESP32-C6 platform
   - Added ADC resolution configuration (12-bit)
   - Initialize both output pins
   - Restore saved output states on boot

4. **storage.cpp**
   - Added save/load for `powerJackState` and `usbOutputState`
   - Updated EEPROM addresses

5. **webserver.cpp**
   - Changed from `ESP8266WebServer` to `WebServer` (ESP32)
   - Updated web UI to show dual outputs
   - Added `/api/powerjack` endpoint
   - Added `/api/usboutput` endpoint
   - Updated `/api/status` to include both VBUS and VOUT
   - Added 5V button to PD voltage control

6. **network.cpp**
   - Changed from `ESP8266WiFi.h` to `WiFi.h`

7. **serial_cmd.cpp**
   - Updated help text for new commands
   - Replaced `/on` and `/off` with `/jack_on`, `/jack_off`, `/usb_on`, `/usb_off`
   - Added `/vbus` and `/vout` commands
   - Updated status display

8. **scheduler.cpp**
   - Modified to control both outputs when schedule triggers

## API Changes

### New Endpoints
- `POST /api/powerjack` - Control power jack output
- `POST /api/usboutput` - Control USB output

### Modified Endpoints
- `GET /api/status` - Now includes `powerJack`, `usbOutput`, `vbus`, and `vout` fields
- `POST /api/pd` - Now supports 5V in addition to 9V, 12V, 15V, 20V

### Removed Endpoints
- `POST /api/power` - Replaced by separate jack and USB endpoints

## Serial Commands

### New Commands
- `/jack_on` - Enable power jack output
- `/jack_off` - Disable power jack output
- `/usb_on` - Enable USB output
- `/usb_off` - Disable USB output
- `/vbus` - Read VBUS voltage
- `/vout` - Read VOUT voltage

### Removed Commands
- `/on` - Replaced by separate jack/USB commands
- `/off` - Replaced by separate jack/USB commands

### Updated Commands
- `/pd <voltage>` - Now accepts 5, 9, 12, 15, or 20
- `/status` - Shows both output states and voltages

## Configuration Notes

### ADC Voltage Divider
The voltage divider ratios are defined in `config.h`:
```cpp
#define VBUS_DIVIDER_RATIO 10.216  // R1=47kΩ, R2=5.1kΩ → (47+5.1)/5.1 = 10.216
#define VOUT_DIVIDER_RATIO 10.216  // R1=47kΩ, R2=5.1kΩ → (47+5.1)/5.1 = 10.216
```

**Hardware Configuration:**
- R1 = 47kΩ (high side)
- R2 = 5.1kΩ (low side to ground)
- Ratio = (R1+R2)/R2 = 52.1/5.1 = 10.216
- Max input voltage = 3.3V × 10.216 ≈ 33.7V (safe for 20V PD)

If you change resistor values, recalculate the ratio and update the constants.

### ADC Configuration
- **Resolution**: 12-bit (0-4095)
- **Reference Voltage**: 3.3V
- **ADC Pins**: GPIO2 and GPIO3

## Testing Checklist

- [ ] Verify button inputs work correctly
- [ ] Test power jack enable/disable
- [ ] Test USB output enable/disable
- [ ] Verify all PD voltages (5V, 9V, 12V, 15V, 20V)
- [ ] Check VBUS voltage reading accuracy
- [ ] Check VOUT voltage reading accuracy
- [ ] Test WiFi connectivity
- [ ] Verify web interface displays correctly
- [ ] Test all API endpoints
- [ ] Verify serial commands work
- [ ] Test schedule functionality
- [ ] Confirm EEPROM save/restore

## Calibration Procedure

1. **Voltage Divider Calibration**:
   - Connect a known voltage source to VBUS
   - Read the value using `/vbus` command
   - Calculate actual ratio: `actual_ratio = known_voltage / measured_voltage`
   - Update `VBUS_DIVIDER_RATIO` in `config.h`
   - Repeat for VOUT

2. **PD Voltage Verification**:
   - Request each PD voltage (5V, 9V, 12V, 15V, 20V)
   - Verify VBUS reads the correct value
   - Check that output voltage is stable

## Safety Notes

⚠️ **Important Safety Considerations**:
- Ensure proper voltage dividers are installed before powering on
- Verify polarity of all connections
- Test with low voltages first
- Monitor temperature of CH224K during operation
- Ensure adequate cooling if running at 20V with high current
