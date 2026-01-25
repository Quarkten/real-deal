# IP Address Display Feature Plan - Version 2

## Overview
Add HTTP endpoints to ESP32 firmware for WiFi control, and create Node.js scripts (.mjs) to interact with ESP32 via HTTP for WiFi scanning, connection, and IP address display.

## Architecture

### 1. ESP32 Firmware Changes ([`esp32/esp32.ino`](esp32/esp32.ino))

#### Add HTTP Endpoints to OTAManager
Extend the existing WebServer in [`ota_manager.h`](esp32/ota_manager.h) to include WiFi control endpoints:

**New Endpoints:**
- `GET /wifi/scan` - Scan available WiFi networks
- `GET /wifi/status` - Get current WiFi connection status and IP address
- `POST /wifi/connect` - Connect to WiFi network (with SSID and password)
- `POST /wifi/save` - Save WiFi credentials to NVS
- `GET /ngrok/url` - Get current Ngrok URL
- `POST /ngrok/url` - Set Ngrok URL

**Response Format:**
```json
{
  "success": true,
  "data": {...},
  "message": "..."
}
```

#### Implementation Details
- Add new route handlers in [`ota_manager.h`](esp32/ota_manager.h)
- Use existing [`wifi_manager.h`](esp32/wifi_manager.h) methods
- Add JSON response helper functions
- Update WebServer initialization to include new routes

### 2. Node.js Scripts (.mjs)

#### IPADDRESS.mjs
**Purpose**: Get and display ESP32's current IP address
**Usage**: `node IPADDRESS.mjs [esp32_ip]`
**Features**:
- Connects to ESP32 via HTTP
- Retrieves current WiFi status and IP
- Displays IP in clear format
- Shows connection status

#### WIFISCAN.mjs
**Purpose**: Scan WiFi networks and display IP
**Usage**: `node WIFISCAN.mjs [esp32_ip]`
**Features**:
- Scans available WiFi networks via ESP32
- Displays network list with signal strength
- Shows current ESP32 IP address
- Provides OTA connection instructions

#### WIFIPASS.mjs
**Purpose**: Connect to WiFi and display IP
**Usage**: `node WIFIPASS.mjs [esp32_ip] [ssid] [password]`
**Features**:
- Prompts for SSID and password if not provided
- Connects ESP32 to WiFi network
- Saves credentials to NVS
- Displays connection status and IP address
- Shows OTA URL for firmware updates

#### NGROKSET.mjs
**Purpose**: Manage Ngrok URL and display IP
**Usage**: `node NGROKSET.mjs [esp32_ip] [new_url]`
**Features**:
- Shows current Ngrok URL
- Displays current ESP32 IP address
- Prompts for new URL if provided
- Updates Ngrok URL in ESP32
- Shows confirmation with IP

### 3. Communication Flow

```
User runs .mjs script
    ↓
Script makes HTTP request to ESP32
    ↓
ESP32 processes request via WebServer
    ↓
ESP32 uses WiFiManager to perform operation
    ↓
ESP32 returns JSON response
    ↓
Script parses response and displays results
```

## Implementation Steps

### Step 1: Modify ESP32 Firmware
- Add HTTP endpoints to [`ota_manager.h`](esp32/ota_manager.h)
- Add JSON response helpers
- Update [`esp32/esp32.ino`](esp32/esp32.ino) to initialize new routes
- Test endpoints with curl/browser

### Step 2: Create Node.js Scripts
- Create [`IPADDRESS.mjs`](IPADDRESS.mjs) - Get IP address
- Create [`WIFISCAN.mjs`](WIFISCAN.mjs) - Scan networks + IP
- Create [`WIFIPASS.mjs`](WIFIPASS.mjs) - Connect WiFi + IP
- Create [`NGROKSET.mjs`](NGROKSET.mjs) - Manage Ngrok + IP

### Step 3: Testing
- Test each script with ESP32
- Verify JSON responses are correct
- Test error handling
- Verify IP display works correctly

### Step 4: Documentation
- Update README with new .mjs script usage
- Add examples for each script
- Document ESP32 IP address for OTA updates

## Benefits
- Users can control ESP32 WiFi from computer without calculator
- Instant IP address display for OTA updates
- No need for serial monitor
- Cross-platform (works on any OS with Node.js)

## Files Modified
- [`esp32/ota_manager.h`](esp32/ota_manager.h) - Add HTTP endpoints
- [`esp32/esp32.ino`](esp32/esp32.ino) - Initialize new routes
- [`IPADDRESS.mjs`](IPADDRESS.mjs) - New file
- [`WIFISCAN.mjs`](WIFISCAN.mjs) - New file
- [`WIFIPASS.mjs`](WIFIPASS.mjs) - New file
- [`NGROKSET.mjs`](NGROKSET.mjs) - New file
