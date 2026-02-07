# IP Address Display Feature - Usage Guide

## Overview

This feature adds HTTP endpoints to the ESP32 firmware and provides Node.js scripts to easily manage WiFi connections, scan networks, and display the ESP32's IP address for OTA updates.

## Files Created

### ESP32 Firmware Changes
- **File**: [`esp32/ota_manager.h`](esp32/ota_manager.h)
- **Changes**: Added HTTP endpoints for WiFi control and Ngrok management
- **New Endpoints**:
  - `GET /wifi/status` - Get WiFi connection status and IP
  - `GET /wifi/scan` - Scan available WiFi networks
  - `POST /wifi/connect` - Connect to WiFi network
  - `POST /wifi/save` - Save WiFi credentials
  - `GET /ngrok/url` - Get current Ngrok URL
  - `POST /ngrok/url` - Set Ngrok URL

### Node.js Scripts

#### 1. IPADDRESS.mjs
**Purpose**: Get and display ESP32's current IP address

**Usage**:
```bash
node IPADDRESS.mjs [esp32_ip]
```

**Example**:
```bash
node IPADDRESS.mjs 192.168.1.100
```

**Output**:
```
=== ESP32 WiFi Status ===
Connected: Yes
IP Address: 192.168.1.100
SSID: MyWiFi
Signal Strength: -45 dBm
==========================

📡 OTA Update Instructions:
👉 Open browser to: http://192.168.1.100/
👉 Upload firmware .bin file
👉 Device will restart automatically
```

#### 2. WIFISCAN.mjs
**Purpose**: Scan WiFi networks and display ESP32 IP address

**Usage**:
```bash
node WIFISCAN.mjs [esp32_ip]
```

**Example**:
```bash
node WIFISCAN.mjs 192.168.1.100
```

**Output**:
```
=== WiFi Network Scan ===
Current IP: 192.168.1.100
Connected: Yes
Current SSID: MyWiFi

Available Networks:
  1. MyWiFi
  2. GuestNetwork
  3. NeighborWiFi

=== OTA Information ===
📡 OTA URL: http://192.168.1.100/
👉 Upload firmware .bin file to update ESP32
========================
```

#### 3. WIFIPASS.mjs
**Purpose**: Connect ESP32 to WiFi network and display IP address

**Usage**:
```bash
node WIFIPASS.mjs [esp32_ip] [ssid] [password]
```

**Example**:
```bash
node WIFIPASS.mjs 192.168.1.100 "MyWiFi" "mypassword"
```

**Interactive Mode** (if SSID/password not provided):
```bash
node WIFIPASS.mjs 192.168.1.100
# Will prompt for SSID and password
```

**Output**:
```
Connecting to WiFi network: MyWiFi
✅ Connected to WiFi network
Saving WiFi credentials...
✅ WiFi credentials saved

=== WiFi Connection Successful ===
SSID: MyWiFi
IP Address: 192.168.1.100
Signal Strength: -45 dBm
==================================

📡 OTA Update Instructions:
👉 Open browser to: http://192.168.1.100/
👉 Upload firmware .bin file
👉 Device will restart automatically
```

#### 4. NGROKSET.mjs
**Purpose**: Manage Ngrok URL and display ESP32 IP address

**Usage**:
```bash
node NGROKSET.mjs [esp32_ip] [new_url]
```

**Example**:
```bash
node NGROKSET.mjs 192.168.1.100 "https://abc123.ngrok-free.app"
```

**Interactive Mode** (if URL not provided):
```bash
node NGROKSET.mjs 192.168.1.100
# Will show current URL and prompt for new URL
```

**Output**:
```
=== Current Configuration ===
ESP32 IP: 192.168.1.100
Connected: Yes
Current Ngrok URL: https://abc123.ngrok-free.app
============================

Do you want to update the Ngrok URL? (y/n): n

=== Final Configuration ===
ESP32 IP: 192.168.1.100
Ngrok URL: https://abc123.ngrok-free.app
==========================

📡 OTA Update Instructions:
👉 Open browser to: http://192.168.1.100/
👉 Upload firmware .bin file
👉 Device will restart automatically
```

## Workflow Examples

### Scenario 1: First-time Setup
```bash
# 1. Connect to ESP32's initial WiFi (if configured)
# 2. Scan available networks
node WIFISCAN.mjs 192.168.1.100

# 3. Connect to your WiFi network
node WIFIPASS.mjs 192.168.1.100 "MyHomeWiFi" "securepassword"

# 4. Set Ngrok URL
node NGROKSET.mjs 192.168.1.100 "https://my-ngrok-url.ngrok-free.app"

# 5. Check IP for OTA updates
node IPADDRESS.mjs 192.168.1.100
```

### Scenario 2: OTA Update
```bash
# 1. Get current IP address
node IPADDRESS.mjs 192.168.1.100

# 2. Open browser to the displayed IP address
# 3. Upload new firmware .bin file
# 4. ESP32 will restart automatically
```

### Scenario 3: Change WiFi Network
```bash
# 1. Connect to ESP32's current WiFi
# 2. Scan available networks
node WIFISCAN.mjs 192.168.1.100

# 3. Connect to new network
node WIFIPASS.mjs 192.168.1.100 "NewNetwork" "newpassword"

# 4. Note the new IP address for future OTA updates
```

## HTTP API Reference

### WiFi Status
```
GET /wifi/status

Response:
{
  "success": true,
  "message": "WiFi status retrieved",
  "data": {
    "connected": true,
    "ipAddress": "192.168.1.100",
    "ssid": "MyWiFi",
    "signalStrength": -45
  }
}
```

### WiFi Scan
```
GET /wifi/scan

Response:
{
  "success": true,
  "message": "WiFi scan completed",
  "data": {
    "networks": ["Network1", "Network2", "Network3"]
  }
}
```

### WiFi Connect
```
POST /wifi/connect
Content-Type: application/x-www-form-urlencoded

Body:
ssid=MyWiFi&password=mypassword

Response:
{
  "success": true,
  "message": "Connected to WiFi network"
}
```

### WiFi Save
```
POST /wifi/save
Content-Type: application/x-www-form-urlencoded

Body:
ssid=MyWiFi&password=mypassword

Response:
{
  "success": true,
  "message": "WiFi credentials saved successfully"
}
```

### Ngrok Get
```
GET /ngrok/url

Response:
{
  "success": true,
  "message": "Ngrok URL retrieved",
  "data": {
    "ngrokUrl": "https://abc123.ngrok-free.app"
  }
}
```

### Ngrok Set
```
POST /ngrok/url
Content-Type: application/x-www-form-urlencoded

Body:
url=https://new-url.ngrok-free.app

Response:
{
  "success": true,
  "message": "Ngrok URL saved successfully"
}
```

## Troubleshooting

### Common Issues

**Issue**: `Cannot find package 'node-fetch'`
**Solution**: Run `npm install node-fetch`

**Issue**: Connection refused / Cannot connect to ESP32
**Solution**: 
- Verify ESP32 is powered on and connected to WiFi
- Check that the IP address is correct
- Ensure your computer is on the same network as the ESP32
- Check if the ESP32's web server is running (port 80)

**Issue**: WiFi connection fails
**Solution**:
- Verify SSID and password are correct
- Ensure the network is 2.4GHz (ESP32 doesn't support 5GHz)
- Check signal strength with WIFISCAN.mjs
- Move ESP32 closer to the router

**Issue**: OTA update fails
**Solution**:
- Verify you're using the correct IP address
- Check that the firmware .bin file is valid
- Ensure ESP32 has enough free space
- Try a different browser

### Debugging Tips

- Use `node IPADDRESS.mjs [ip]` to verify connectivity
- Check ESP32 serial monitor for detailed logs
- Test with `curl` commands to verify HTTP endpoints
- Ensure no firewall is blocking port 80

## Integration with Existing System

The new HTTP endpoints work alongside the existing calculator-based DBus commands:

- **Calculator programs** still use DBus protocol (commands 15-19)
- **Node.js scripts** use HTTP endpoints (new feature)
- **OTA updates** continue to work as before
- **WiFi configuration** can be done via calculator OR scripts

## Security Notes

- The HTTP endpoints are only accessible on the local network
- No authentication is required for local access
- Sensitive data (WiFi passwords) are transmitted in plaintext
- For production use, consider adding authentication

## Future Enhancements

- Add authentication for HTTP endpoints
- Implement network discovery to find ESP32 automatically
- Add signal strength indicators in WIFISCAN output
- Support for multiple saved WiFi networks
- Web-based configuration interface