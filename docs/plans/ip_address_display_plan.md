# IP Address Display Feature Plan

## Overview
Add ability to display ESP32's local IP address from calculator programs and server, similar to WIFISCAN functionality. This enables users to quickly find the IP for OTA updates without checking serial monitor.

## Architecture

### 1. ESP32 Changes

#### New Command Handler (Command ID 20)
- **Function**: `get_ip_address()`
- **Location**: [`esp32/esp32.ino`](esp32/esp32.ino)
- **Behavior**: Returns ESP32's current WiFi IP address
- **Return Format**: String in format `XXX.XXX.XXX.XXX` or error message

#### Command Array Entry
```cpp
{ 20, "get_ip_address", 0, get_ip_address, false }
```

#### Implementation Details
- Uses existing [`wifi_manager.h`](esp32/wifi_manager.h) `getIPAddress()` method
- No WiFi connection required (returns 0.0.0.0 if disconnected)
- Sets success message with IP string
- Integrates with existing message passing system

### 2. Calculator Programs

#### WIFISCAN Enhancement
- **File**: `programs/WIFISCAN.8xp` (source embedded in docs)
- **New Feature**: Display IP address after showing available networks
- **Flow**:
  1. Send command 15 (scan_networks)
  2. Get network list
  3. Display networks to user
  4. Send command 20 (get_ip_address)
  5. Display current IP
  6. Show connection status

#### WIFIPASS Enhancement
- **File**: `programs/WIFIPASS.8xp`
- **New Feature**: Display IP after successful connection
- **Flow**:
  1. Prompt for SSID and password
  2. Send command 16 (connect_wifi)
  3. Send command 17 (save_wifi)
  4. Get IP via command 20
  5. Display "Connected to [SSID], IP: [IP]"

#### NGROKSET Enhancement
- **File**: `programs/NGROKSET.8xp`
- **New Feature**: Display IP when showing current/new Ngrok URL
- **Flow**:
  1. Get current Ngrok URL via command 18
  2. Get current IP via command 20
  3. Display current settings
  4. Prompt for new URL if desired
  5. Update and display confirmation with IP

### 3. Server Integration (chatgpt.mjs)

#### Enhancement
- **File**: [`server/routes/chatgpt.mjs`](server/routes/chatgpt.mjs)
- **New Endpoint**: Display IP information in startup/help messages
- **Purpose**: Provide OTA connection instructions with auto-detected IP

## Implementation Steps

### Step 1: Add ESP32 Command Handler
- Add `get_ip_address()` function to [`esp32/esp32.ino`](esp32/esp32.ino)
- Add command entry to commands array
- Update MAXCOMMAND constant to 20

### Step 2: Update Calculator Programs
- Modify WIFISCAN to include IP display
- Modify WIFIPASS to include IP display after connection
- Modify NGROKSET to include IP display in status

### Step 3: Server Updates
- Update startup/help output in chatgpt.mjs to show IP instructions
- Add OTA URL hints with IP address format

### Step 4: Testing
- Verify command 20 returns correct IP when WiFi connected
- Verify command 20 returns 0.0.0.0 when WiFi disconnected
- Test each calculator program displays IP correctly
- Verify OTA connection instructions are clear

## Benefits
- Users can instantly see ESP32 IP without serial monitor
- Seamless workflow for WiFi configuration
- Clear OTA update instructions
- No additional hardware needed

## Files Modified
- `esp32/esp32.ino` - Add command handler
- `programs/WIFISCAN.8xp` - Add IP display
- `programs/WIFIPASS.8xp` - Add IP display
- `programs/NGROKSET.8xp` - Add IP display
- `server/routes/chatgpt.mjs` - Add IP info to help text
