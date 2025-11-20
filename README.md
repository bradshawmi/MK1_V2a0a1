# Arc Reactor MK1 - WebUI Connectivity Options

This document explains the connectivity options available for the Arc Reactor MK1 WebUI on the Xiao ESP32C3 board.

## Current Implementation: WiFi-based WebUI

The Arc Reactor MK1 currently uses **WiFi** to serve the WebUI through an HTTP web server. Here's how it works:

- **Access Point Mode**: The ESP32C3 creates a WiFi access point (SSID: `ArcReactorMK1`, Password: `arcreactor`)
- **Web Server**: An HTTP server runs on port 80
- **User Interface**: Users connect to the WiFi network and access the WebUI through a web browser at the device's IP address
- **Full HTML/CSS/JavaScript**: The interface is served as a complete web page with interactive controls

## Can the WebUI Work Over Bluetooth?

**Short Answer**: No, the current HTTP-based WebUI cannot run over Bluetooth in its present form.

### Technical Explanation

#### ESP32C3 Bluetooth Capabilities
- The Xiao ESP32C3 supports **Bluetooth 5.0 Low Energy (BLE)** only
- It does **NOT** support Bluetooth Classic (unlike the original ESP32)
- BLE is designed for low-power, low-bandwidth data exchanges

#### Why HTTP Over Bluetooth Isn't Practical

1. **No TCP/IP Stack**: Bluetooth (both Classic and BLE) doesn't natively support TCP/IP networking, which HTTP requires
2. **Protocol Mismatch**: 
   - HTTP servers rely on TCP/IP connections
   - Bluetooth Classic uses profiles like SPP (Serial Port Profile) for data
   - BLE uses GATT (Generic Attribute Profile) for small data packets
3. **Architecture Incompatibility**: The current WebServer library is built for TCP/IP networks (WiFi/Ethernet), not Bluetooth

## Alternative: Web Bluetooth API (Recommended)

While you cannot serve the WebUI over Bluetooth, you **can** create a browser-based control interface that communicates with the ESP32C3 via **Web Bluetooth API**.

### How Web Bluetooth Works

```
┌─────────────────┐         BLE/GATT          ┌──────────────────┐
│  ESP32C3 Device │ ◄─────────────────────► │  Web Browser     │
│  (BLE Server)   │   Characteristic R/W     │  (Chrome/Edge)   │
└─────────────────┘                          └──────────────────┘
                                                      │
                                              ┌───────▼──────────┐
                                              │  HTML/CSS/JS UI  │
                                              │  (Web Bluetooth  │
                                              │   JavaScript)    │
                                              └──────────────────┘
```

### Implementation Approach

1. **ESP32C3 Side** (C++/Arduino):
   - Set up BLE server with custom GATT services
   - Create characteristics for each control (brightness, colors, effects, etc.)
   - Handle read/write operations on characteristics

2. **Browser Side** (HTML/JavaScript):
   - Create a web page (can be hosted anywhere or opened locally)
   - Use Web Bluetooth JavaScript API to connect to the ESP32C3
   - Read/write characteristic values to control the device
   - Display current state in the UI

### Example BLE Characteristic Structure

```
Service: Arc Reactor Control (UUID: custom)
├── Characteristic: Master Brightness (read/write, uint8)
├── Characteristic: Zone 0 Effect A (read/write, string)
├── Characteristic: Zone 0 Color A (read/write, 24-bit RGB)
├── Characteristic: Auto Dying Flicker (read/write, bool)
└── ... (other controls)
```

### Browser Support

Web Bluetooth API is supported in:
- ✅ Chrome (Desktop and Android)
- ✅ Edge (Desktop and Android)
- ✅ Opera (Desktop and Android)
- ❌ Safari (iOS/macOS) - Not supported
- ❌ Firefox - Limited/experimental support

## Comparison: WiFi vs BLE

| Feature | WiFi (Current) | BLE (Web Bluetooth) |
|---------|---------------|---------------------|
| Full HTML WebUI | ✅ Yes | ❌ No (requires separate webpage) |
| Range | ~30-50m | ~10-30m |
| Power Consumption | Higher | Lower |
| Browser Support | All browsers | Chrome/Edge/Opera only |
| Implementation Complexity | Simple (standard HTTP) | Moderate (BLE GATT + JS) |
| Real-time Updates | WebSocket/Polling | BLE notifications |
| Multi-client Support | Yes (multiple browsers) | Limited (typically 1 connection) |
| No WiFi Network Required | No | Yes |

## Recommendations

### For Current Use Case
**Keep WiFi**: The current WiFi-based implementation is the best choice because:
- Full-featured web interface with rich HTML/CSS/JavaScript
- Works with all browsers
- Well-established, simple implementation
- Multiple users can access simultaneously
- Easier debugging and development

### When to Consider BLE
Consider implementing BLE if:
- You need lower power consumption
- You want to avoid WiFi network congestion
- You're building a dedicated mobile app (native iOS/Android)
- You only need simple on/off controls and parameter adjustments
- Privacy is important (BLE has shorter range than WiFi)

### Hybrid Approach
You could implement **both** WiFi and BLE:
- WiFi for full WebUI access (primary interface)
- BLE for quick adjustments and low-power remote control
- User can choose which interface to use based on their needs

## Implementation Resources

If you want to add BLE support alongside WiFi, here are helpful resources:

### Tutorials
- [ESP32 Web Bluetooth (BLE) Guide](https://randomnerdtutorials.com/esp32-web-bluetooth/)
- [Seeed Studio XIAO ESP32C3 Bluetooth Usage](https://wiki.seeedstudio.com/XIAO_ESP32C3_Bluetooth_Usage/)
- [ESP32 BLE Server and Client](https://randomnerdtutorials.com/esp32-ble-server-client/)

### Arduino Libraries
- `BLE` (built-in with ESP32 Arduino core)
- `BLEServer`, `BLEDevice`, `BLEUtils` classes

### Example Code Structure
```cpp
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Define custom UUIDs for your services
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

void setupBLE() {
  BLEDevice::init("ArcReactorMK1");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  
  pCharacteristic->setValue("Hello from Arc Reactor");
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
}
```

## Conclusion

**Answer to Original Question**: The WebUI cannot work over Bluetooth in its current HTTP-based form. However, you have two options:

1. **Continue using WiFi** (recommended) - Full-featured, works great
2. **Implement Web Bluetooth API** - Create a BLE-based control interface as an alternative to the HTTP WebUI

The WiFi-based WebUI is the most practical and feature-rich solution for this project. BLE would require a complete redesign of the communication architecture and would have limitations compared to the current implementation.

---

## Current WiFi Access Instructions

To access the WebUI:
1. Power on the Arc Reactor MK1
2. Connect to WiFi network: `ArcReactorMK1` (password: `arcreactor`)
3. Open a web browser
4. Navigate to the device's IP address (typically `192.168.4.1`)
5. Use the full-featured HTML interface to control all aspects of the device

## Button Controls

- **Short tap**: Restore WiFi (if auto-off is enabled)
- **Long press**: Sleep/Wake
- **Double tap**: Cycle to next preset

---

*For questions or feature requests, please open an issue on GitHub.*
