# Connectivity FAQ - Arc Reactor MK1

## Quick Answers

### Q: Can I use the WebUI over Bluetooth instead of WiFi?

**A: No, but there's an alternative.**

The current WebUI uses HTTP over WiFi and cannot work over Bluetooth. However, you could implement a Web Bluetooth interface as an alternative (see [README.md](README.md) for details).

### Q: Why doesn't Bluetooth work with the WebUI?

**A: Protocol incompatibility.**

- The WebUI is an HTTP server that requires TCP/IP networking
- Bluetooth (both Classic and BLE) doesn't support TCP/IP
- ESP32C3 only has BLE (not Bluetooth Classic)
- BLE uses GATT for small data packets, not HTTP requests/responses

### Q: What Bluetooth features does ESP32C3 have?

**A: Bluetooth 5.0 Low Energy (BLE) only.**

- ✅ Bluetooth Low Energy (BLE)
- ❌ Bluetooth Classic (not supported)
- ❌ Bluetooth SPP (Serial Port Profile - requires Classic)
- ❌ Bluetooth A2DP (Audio - requires Classic)

### Q: Can I control the device from my phone over Bluetooth?

**A: Yes, but requires custom implementation.**

Options:
1. **Web Bluetooth** - Works in Chrome on Android, requires custom HTML/JS page
2. **Native App** - Build iOS/Android app using BLE libraries
3. **WiFi (Current)** - Use any browser on any device

### Q: What's the recommended way to control the device?

**A: WiFi (current implementation).**

The WiFi-based WebUI is the best choice because:
- Works with all browsers and devices
- Full-featured HTML interface
- Easier to use and debug
- Better range than BLE
- Multiple users can connect simultaneously

### Q: How do I access the WiFi WebUI?

**A: Connect to the WiFi network, then open browser.**

1. Power on device
2. Connect to WiFi: `ArcReactorMK1` (password: `arcreactor`)
3. Open browser to `192.168.4.1`

### Q: Does using WiFi drain the battery faster than Bluetooth would?

**A: Yes, but the difference is minimal for this use case.**

- WiFi uses more power than BLE
- However, the LED controller uses much more power than either radio
- WiFi has idle auto-off feature to save power
- Battery impact from WiFi is negligible compared to LED operation

### Q: Can I use both WiFi and Bluetooth at the same time?

**A: Technically yes, but requires implementation.**

The ESP32C3 can run both WiFi and BLE simultaneously. You could implement:
- WiFi for full WebUI (primary interface)
- BLE for quick controls (secondary interface)

This would require significant code changes to add BLE functionality alongside the existing WiFi WebUI.

### Q: Is my WiFi connection secure?

**A: It uses WPA2 with a password.**

- Access Point mode with WPA2 encryption
- Default password: `arcreactor`
- **Recommendation**: Change the password in the code if security is a concern
- The device doesn't connect to your home WiFi, it creates its own network

### Q: Can I connect the device to my home WiFi instead of using AP mode?

**A: Not in the current implementation, but this is possible.**

The current code runs in Access Point (AP) mode only. You could modify it to:
- Connect to your home WiFi (Station mode)
- Access the WebUI from any device on your home network
- No need to switch WiFi networks on your phone/tablet

This would require code changes to add WiFi station mode configuration.

### Q: What's the WiFi range?

**A: Approximately 30-50 meters (100-165 feet) in open space.**

Range depends on:
- Physical obstacles (walls, furniture)
- Other WiFi networks (interference)
- Device orientation
- Environmental factors

### Q: Can multiple people control the device at once?

**A: Yes with WiFi, limited with BLE.**

- **WiFi (Current)**: Multiple browsers can connect and control simultaneously
- **BLE (If Implemented)**: Typically only 1 device can connect at a time

### Q: Does the device need internet access?

**A: No.**

- The device creates its own WiFi network (Access Point mode)
- No internet connection required
- All HTML/CSS/JavaScript is served from the device itself
- Works completely offline

### Q: What browsers are compatible?

**With current WiFi WebUI:**
- ✅ All modern browsers (Chrome, Safari, Firefox, Edge, etc.)
- ✅ Mobile browsers (iOS, Android)
- ✅ Desktop browsers (Windows, Mac, Linux)

**If Web Bluetooth was implemented:**
- ✅ Chrome (Desktop and Android)
- ✅ Edge (Desktop and Android)
- ✅ Opera (Desktop and Android)
- ❌ Safari (iOS/macOS)
- ❌ Firefox (limited support)

### Q: Can I use the device while charging?

**A: Yes.**

The device works normally while connected to USB power for charging.

### Q: How do I turn WiFi on/off to save battery?

**A: It has automatic idle timeout.**

- WiFi automatically turns off after 5 minutes of inactivity (configurable)
- Short button press turns WiFi back on
- You can disable auto-off in the WebUI settings

### Q: What happens if I lose WiFi connection?

**A: The device continues running.**

- LED effects keep running
- Last settings are saved in flash memory
- WiFi can be re-enabled with a button press
- Settings persist across power cycles

## Technical Details

### Current Architecture
```
User Device              ESP32C3 Device
(Phone/Tablet/PC)       (Arc Reactor MK1)
      │                         │
      │   WiFi (2.4GHz)        │
      │◄──────────────────────►│
      │                         │
      │   HTTP (Port 80)       │
      │◄──────────────────────►│
      │                         │
   Browser                 WebServer
  (HTML/CSS/JS)         (serves WebUI)
```

### If BLE Was Implemented
```
User Device              ESP32C3 Device
(Phone/Tablet/PC)       (Arc Reactor MK1)
      │                         │
      │   BLE (GATT)           │
      │◄──────────────────────►│
      │                         │
   Browser                 BLE Server
  (Web Bluetooth JS)    (GATT Characteristics)
```

---

## Need More Information?

See [README.md](README.md) for:
- Detailed technical explanation
- Web Bluetooth implementation guide
- Comparison table: WiFi vs BLE
- Code examples for BLE implementation
- Links to tutorials and resources

## Found a Bug or Have a Question?

Please open an issue on GitHub: https://github.com/bradshawmi/MK1_V2a0a1/issues
