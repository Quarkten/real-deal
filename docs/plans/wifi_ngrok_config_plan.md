# WiFi & Ngrok Configuration Over-the-Air (OTA) Plan

## 📋 Overview

Enable users to configure WiFi networks and ngrok URLs directly from the TI-84 Plus calculator without physically accessing the ESP32. All credentials will be stored in non-volatile storage (NVS) and persist across reboots. Additionally, enable firmware updates via web browser without needing to physically reconnect the Pico programmer.

## 🏗️ Complete System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    YOUR COMPUTER (Node.js Server)               │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ Node.js Server (index.mjs)                              │  │
│  │ - ChatGPT Routes (/gpt/ask)                             │  │
│  │ - Image Routes (/image/*)                               │  │
│  │ - Programs Routes (/programs/*)                          │  │
│  │ - Chats Routes (/chats/*)                                │  │
│  │ - Listens on port 8080 (localhost:8080)                 │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ Ngrok Tunnel (ngrok.exe)                                │  │
│  │ - Creates public HTTPS URL                              │  │
│  │ - Tunnels: localhost:8080 → https://abc123.ngrok...    │  │
│  │ - Example: https://c7532afaf9b0.ngrok-free.app         │  │
│  └──────────────────────────────────────────────────────────┘  │
│                            │                                    │
│                    (Public HTTPS Tunnel)                        │
│                            │                                    │
└────────────────────────────┼────────────────────────────────────┘
                             │ (WiFi)
                             ▼
┌────────────────────────────────────────────────────────────────┐
│              HOME WIFI NETWORK (Moonlight2.5)                   │
│                                                                │
│  ┌───────────────────┐              ┌──────────────────────┐ │
│  │ TI-84 Plus        │              │ ESP32-CAM            │ │
│  │ (via DBus/Serial) │◄────────────►│ (WiFi Connected)     │ │
│  │                   │    GPIO12/13  │                      │ │
│  │ • WiFi Scanning   │              │ • WiFi Manager       │ │
│  │ • Password Entry  │              │ • NVS Storage        │ │
│  │ • Ngrok URL Mgmt  │              │ • Web Server (OTA)   │ │
│  │ • Status Display  │              │ • Config Manager     │ │
│  └───────────────────┘              └──────────────────────┘ │
│                                                                │
│                                      IP: 192.168.1.XXX        │
│                                      Port: 80 (Web OTA)       │
│                                      Port: HTTPS (API calls)  │
└────────────────────────────────────────────────────────────────┘
```

## 🔄 Data Flow Architecture

### Initial Setup (First Boot with Pico)
```
1. Flash ESP32 with initial firmware via Pico programmer
2. ESP32 boots with hardcoded WiFi (secrets.h) temporarily
3. ESP32 connects to home WiFi network
4. Calculator connects to ESP32 via serial cable (GPIO 12/13)
```

### WiFi Reconfiguration (From Calculator)
```
Calculator                   ESP32                    WiFi Network
    │                           │                           │
    ├─ Scan WiFi ──────────────►│                           │
    │                           ├─ Scan networks ──────────►│
    │                           │◄─ Return SSID list ───────┤
    │◄─ Display Menu ───────────┤                           │
    │                           │                           │
    │ [User selects network]    │                           │
    │                           │                           │
    ├─ Enter Password ──────────►│                           │
    │                           ├─ Try Connect ────────────►│
    │                           │◄─ Connected! ────────────┤
    │◄─ Success/Fail ───────────┤                           │
    │                           ├─ Save to NVS (persistent)│
    │                           │                           │
```

### Ngrok URL Update (From Calculator)
```
Calculator               ESP32                    Node.js Server
    │                      │                           │
    ├─ Send new URL ─────►│                           │
    │ (via serial)        ├─ Save to NVS              │
    │                     ├─ Update global SERVER var │
    │◄─ Status/Confirm ──┤                           │
    │                     ├─ Next API call uses new URL
    │                     ├─ Connect via Ngrok ─────►│
    │                     │◄─ Response ──────────────┤
```

### OTA Firmware Update (From Web Browser)
```
Your Computer          ESP32 Web Server         ESP32 Flash Memory
    │                      │                           │
    ├─ Open browser ───►│                           │
    │ http://192.168.1.XXX
    │                      ├─ Show OTA page           │
    │◄─ HTML form ───────┤                           │
    │                      │                           │
    │ [Select .bin file]  │                           │
    │ [Click Upload]      │                           │
    │                      │                           │
    ├─ POST /update ────►│                           │
    │ (multipart .bin)    ├─ Receive firmware        │
    │                     ├─ Write to flash ────────►│
    │                     │◄─ Complete ──────────────┤
    │◄─ "Update OK" ─────┤                           │
    │                     ├─ Restart ESP32           │
    │                     │                           │
```

## 📝 Command Structure

### New Commands Added (IDs 15-19)

| Cmd ID | Name | Args | Returns | Purpose |
|--------|------|------|---------|---------|
| 15 | `scan_networks` | 0 | String | Return formatted list of available WiFi networks |
| 16 | `connect_wifi` | 2 | Status | Connect to SSID (Str0) with password (Str1) |
| 17 | `save_wifi` | 2 | Status | Save WiFi credentials to NVS for persistence |
| 18 | `get_ngrok` | 0 | String | Retrieve current ngrok URL from NVS |
| 19 | `set_ngrok` | 1 | Status | Update ngrok URL in NVS (Str0) |

## 🔧 ESP32 Implementation Details

### Phase 1: Modify esp32.ino
1. Add `#include <WiFiScan.h>` for network scanning
2. Create `wifi_manager.h` for WiFi operations
3. Create `config_manager.h` for NVS operations
4. Create `ota_manager.h` for web-based firmware updates
5. Add new command handlers (15-19)
6. Update startup sequence to load from NVS
7. Start web server on port 80 for OTA interface

### Phase 2: Update secrets.h Strategy
- Keep as fallback for initial setup (first boot with Pico)
- Read from NVS on subsequent boots
- Provide factory reset command if needed

### Phase 3: TI-BASIC Programs
Three new calculator programs:
- **WIFISCAN.8xp** - Display available networks, user selects one
- **WIFIPASS.8xp** - Prompt for password, attempt connection, show status
- **NGROKSET.8xp** - Prompt for new ngrok URL, update and verify

### Phase 4: Web OTA Interface
- Simple HTML form at `http://192.168.1.XXX/update`
- Accepts .bin firmware files
- Shows upload progress
- Automatic restart after update

## 🗂️ File Structure

```
esp32/
├── esp32.ino                 (MODIFIED - add command routing + web server)
├── config.h                  (NEW - command IDs and constants)
├── secrets.h                 (EXISTING - fallback values)
├── wifi_manager.h            (NEW - WiFi scanning & connection)
├── config_manager.h          (NEW - NVS operations)
├── ota_manager.h             (NEW - Web OTA interface)
├── launcher.h                (EXISTING - unchanged)
├── camera_index.h            (EXISTING - unchanged)
└── camera_pins.h             (EXISTING - unchanged)

programs/
├── WIFISCAN.8xp             (NEW - TI-BASIC)
├── WIFIPASS.8xp             (NEW - TI-BASIC)
├── NGROKSET.8xp             (NEW - TI-BASIC)
└── LAUNCHER.8xp             (EXISTING - unchanged)

server/
├── index.mjs                (EXISTING - Node.js backend)
├── ngrok.exe                (EXISTING - Ngrok tunnel)
├── package.json             (EXISTING - dependencies)
└── routes/                  (EXISTING - API endpoints)
```

## 💾 NVS Storage Schema

```cpp
// Namespace: "config" (max 16 bytes for namespace)
// Keys:
//   "wifi_ssid"      → String, max 32 bytes
//   "wifi_pass"      → String, max 64 bytes
//   "ngrok_url"      → String, max 256 bytes
//   "wifi_connected" → UInt, 0/1 boolean
//   "boot_count"     → UInt, persisted reboot counter
```

## 🔒 Security Considerations

1. **Credentials in NVS**: Encrypted by ESP32's flash encryption (if enabled)
2. **Password Masking**: Calculator doesn't echo password during entry
3. **Unlock Protocol**: Existing password (42069) still required for safety
4. **Validation**: Check SSID exists before saving, validate ngrok URL format
5. **Fallback**: If NVS is corrupted, fallback to hardcoded secrets.h
6. **OTA Authentication**: Optional - can add basic auth to web OTA interface
7. **HTTPS Only**: Ngrok tunnel uses HTTPS for API calls to Node.js server

## 🚀 Implementation Sequence

1. **Create header files** (wifi_manager.h, config_manager.h, ota_manager.h, config.h)
2. **Implement NVS operations** (read/write to flash storage)
3. **Implement WiFi scanning** (list available networks)
4. **Add command handlers** (15-19 in esp32.ino)
5. **Update startup sequence** (load from NVS, fallback to secrets.h)
6. **Create TI-BASIC programs** (three new calculator apps)
7. **Implement web OTA interface** (simple form + file upload handler)
8. **Testing & validation**
9. **Update README.md** with complete setup and usage instructions

## ✅ Success Criteria

- [ ] User can see list of available WiFi networks from calculator
- [ ] User can select network and enter password via calculator
- [ ] Credentials persist across ESP32 reboots (stored in NVS)
- [ ] User can update ngrok URL from calculator without physical access
- [ ] User can update ESP32 firmware via web browser (no Pico needed)
- [ ] All operations show status feedback on calculator
- [ ] Existing functionality (GPT, programs, images) still works
- [ ] Factory reset option available if configuration corrupted
- [ ] Node.js server stays running and Ngrok tunnel maintains connection
- [ ] Calculator can query updated ngrok URL immediately after change

## 🔌 Integration Points

- **TICL/CBL2 Library**: Existing serial protocol, no changes needed
- **WiFi Library**: Built-in to ESP32, uses WiFi.scanNetworks()
- **Preferences Library**: Built-in NVS wrapper (already used in code)
- **HTTPClient**: Updated SERVER URL from NVS before requests
- **WebServer Library**: Built-in to ESP32, hosts OTA interface
- **Arduino IDE**: Existing build system, no new dependencies

## 📚 Setup Instructions for README.md

### Prerequisites
- Node.js 16+ installed
- Ngrok account (free tier available at https://ngrok.com)
- ESP32-CAM with initial firmware already flashed via Pico

### First Time Setup

1. **Flash ESP32-CAM with Pico** (one-time)
   - Use existing Pico programmer method
   - ESP32 boots with default WiFi from secrets.h

2. **Start Node.js Server**
   ```bash
   cd server
   npm install
   npm start
   ```
   - Server listens on http://localhost:8080

3. **Start Ngrok Tunnel**
   ```bash
   cd server
   ./ngrok.exe http 8080
   ```
   - Shows public URL: `https://xxxxx.ngrok-free.app`
   - Copy this URL for next step

4. **Configure WiFi & Ngrok from Calculator**
   - Connect calculator to ESP32 via serial cable
   - Run WIFISCAN program → Select new network
   - Run WIFIPASS program → Enter WiFi password
   - Run NGROKSET program → Enter Ngrok URL from step 3
   - ESP32 reboots and connects to new WiFi

5. **Future Firmware Updates**
   - Open browser: `http://192.168.1.XXX/update` (find local IP from serial monitor)
   - Upload new .bin file
   - ESP32 updates and restarts automatically

