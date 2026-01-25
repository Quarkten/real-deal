# Power Management Implementation Plan

## Overview

Implement software-only power management using existing TIP/RING pins to detect calculator connection and automatically enter deep sleep when power is lost.

## Requirements

### Functional Requirements
1. **Power Detection**: Monitor TIP/RING pins for voltage presence
2. **Deep Sleep**: Enter deep sleep after configurable delay when power is lost
3. **Wake-up**: Wake from deep sleep when power is restored
4. **WiFi Reconnection**: Reconnect to WiFi only when needed (after wake-up)
5. **State Preservation**: Maintain WiFi credentials and configuration

### Non-Functional Requirements
1. **No Hardware Changes**: Use existing TIP/RING connections only
2. **Low Power**: Minimize power consumption in deep sleep (~10µA)
3. **Fast Wake-up**: Quick recovery when calculator reconnects
4. **Backward Compatible**: Don't break existing functionality

## Implementation Strategy

### 1. Power Detection

**Method**: Monitor existing TIP/RING pins (GPIO 12 and 13)
- When calculator connected: Pins read HIGH (voltage present)
- When calculator disconnected: Pins read LOW (no voltage)
- Use digitalRead() for simple HIGH/LOW detection

**Advantages**:
- No additional hardware needed
- Uses existing connections
- Simple and reliable

**Limitations**:
- Less precise than analog voltage measurement
- May need calibration for different calculators

### 2. Deep Sleep Configuration

**Wake-up Sources**:
- External wake-up via GPIO (TIP/RING pins)
- Timer wake-up (optional, for periodic checks)

**Deep Sleep Mode**:
- Disable WiFi radio
- Disable CPU and peripherals
- Keep RTC memory for state
- Minimal power consumption

### 3. State Management

**RTC Memory**:
- Store boot count
- Store WiFi connection state
- Store last known IP address
- Store power state

**NVS Storage**:
- WiFi credentials (already implemented)
- Configuration parameters
- Power management settings

## Code Implementation

### New Files/Modifications

#### 1. esp32/esp32.ino

**Add Power Management Functions**:
```cpp
// Power management configuration
#define POWER_PIN_TIP 12
#define POWER_PIN_RING 13
#define POWER_CHECK_INTERVAL 100  // ms
#define POWER_LOSS_DELAY 5000     // ms (5 seconds delay before deep sleep)

// State variables
bool isPowered = false;
bool wasPowered = false;
unsigned long lastPowerCheck = 0;
unsigned long powerLossStartTime = 0;
bool powerLossDetected = false;

// RTC memory for state preservation
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool wifiConnected = false;
RTC_DATA_ATTR char lastIP[16] = {0};

void setupPowerManagement() {
  Serial.println("[Power] Setting up power management...");
  
  // Configure TIP/RING pins as inputs
  pinMode(POWER_PIN_TIP, INPUT);
  pinMode(POWER_PIN_RING, INPUT);
  
  // Read initial power state
  isPowered = checkPowerPresence();
  wasPowered = isPowered;
  
  Serial.print("[Power] Initial power state: ");
  Serial.println(isPowered ? "CONNECTED" : "DISCONNECTED");
  
  // Restore state from RTC memory
  restoreStateFromRTC();
}

bool checkPowerPresence() {
  // Read voltage on both pins
  int tipValue = digitalRead(POWER_PIN_TIP);
  int ringValue = digitalRead(POWER_PIN_RING);
  
  // Power is present if either pin is HIGH
  // (Calculator may only drive one pin at a time)
  return (tipValue == HIGH) || (ringValue == HIGH);
}

void handlePowerState() {
  bool currentPower = checkPowerPresence();
  
  if (currentPower != isPowered) {
    if (currentPower) {
      // Power restored
      Serial.println("[Power] Power restored - waking up");
      powerLossDetected = false;
      powerLossStartTime = 0;
      wakeUp();
    } else {
      // Power lost
      if (!powerLossDetected) {
        Serial.println("[Power] Power lost - starting delay timer");
        powerLossDetected = true;
        powerLossStartTime = millis();
      }
    }
    isPowered = currentPower;
  }
  
  // Check if delay has expired
  if (powerLossDetected && (millis() - powerLossStartTime > POWER_LOSS_DELAY)) {
    Serial.println("[Power] Delay expired - entering deep sleep");
    enterDeepSleep();
  }
}

void enterDeepSleep() {
  Serial.println("[Power] Enterring deep sleep...");
  
  // Save state to RTC memory
  saveStateToRTC();
  
  // Disable WiFi to save power
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // Configure wake-up on GPIO change
  // Wake when TIP or RING pin goes HIGH (calculator reconnected)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_PIN_TIP, 1);
  esp_sleep_enable_ext1_wakeup(
    (1ULL << POWER_PIN_TIP) | (1ULL << POWER_PIN_RING),
    ESP_EXT1_WAKEUP_ANY_HIGH
  );
  
  // Enter deep sleep
  Serial.println("[Power] Entering deep sleep...");
  esp_deep_sleep_start();
}

void wakeUp() {
  Serial.println("[Power] Waking up from deep sleep...");
  
  // Reinitialize WiFi
  WiFi.mode(WIFI_STA);
  
  // Reconnect to WiFi if credentials are saved
  String ssid = configMgr.getSsid();
  String pass = configMgr.getPassword();
  
  if (ssid.length() > 0 && pass.length() > 0) {
    Serial.print("[Power] Reconnecting to WiFi...");
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    // Wait for connection with timeout
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[Power] WiFi reconnected");
      Serial.print("[Power] IP: ");
      Serial.println(WiFi.localIP());
      wifiConnected = true;
      strncpy(lastIP, WiFi.localIP().toString().c_str(), 15);
    } else {
      Serial.println("\n[Power] WiFi reconnection failed");
      wifiConnected = false;
    }
  } else {
    Serial.println("[Power] No WiFi credentials saved");
    wifiConnected = false;
  }
  
  // Restart OTA server
  otaMgr.begin();
  
  // Reset power loss detection
  powerLossDetected = false;
  powerLossStartTime = 0;
}

void saveStateToRTC() {
  if (WiFi.isConnected()) {
    wifiConnected = true;
    strncpy(lastIP, WiFi.localIP().toString().c_str(), 15);
  } else {
    wifiConnected = false;
  }
}

void restoreStateFromRTC() {
  bootCount++;
  Serial.print("[Power] Boot count: ");
  Serial.println(bootCount);
  
  if (wifiConnected && strlen(lastIP) > 0) {
    Serial.print("[Power] Last known IP: ");
    Serial.println(lastIP);
  }
}
```

**Modify setup() Function**:
```cpp
void setup() {
  Serial.begin(115200);
  Serial.println("[CBL]");
  
  // Setup power management first
  setupPowerManagement();
  
  // Check if we should skip normal setup (in deep sleep)
  if (!isPowered) {
    Serial.println("[Power] No power detected, entering deep sleep immediately");
    enterDeepSleep();
    return;  // Never reached
  }
  
  // Continue with existing setup...
  cbl.setLines(TIP, RING);
  cbl.resetLines();
  cbl.setupCallbacks(header, data, MAXDATALEN, onReceived, onRequest);
  
  pinMode(TIP, INPUT);
  pinMode(RING, INPUT);
  
  // ... rest of existing setup code
}
```

**Modify loop() Function**:
```cpp
void loop() {
  // Check power state periodically
  if (millis() - lastPowerCheck > POWER_CHECK_INTERVAL) {
    handlePowerState();
    lastPowerCheck = millis();
  }
  
  // Only run main logic if powered
  if (isPowered) {
    // Handle OTA web server
    otaMgr.handleClient();
    
    // Handle queued actions
    if (queued_action) {
      delay(1000);
      Serial.println("executing queued actions");
      void (*tmp)() = queued_action;
      queued_action = NULL;
      tmp();
    }
    
    // Process commands
    if (command >= 0 && command <= MAXCOMMAND) {
      for (int i = 0; i < NUMCOMMANDS; ++i) {
        if (commands[i].id == command && commands[i].num_args == currentArg) {
          if (commands[i].wifi && !WiFi.isConnected()) {
            setError("wifi not connected");
          } else {
            Serial.print("processing command: ");
            Serial.println(commands[i].name);
            commands[i].command_fp();
          }
        }
      }
    }
    
    // Handle DBus events
    cbl.eventLoopTick();
  } else {
    // In deep sleep mode, minimal processing
    delay(100);
  }
}
```

#### 2. esp32/config.h

**Add Power Management Configuration**:
```cpp
// Power Management Configuration
#define POWER_PIN_TIP 12
#define POWER_PIN_RING 13
#define POWER_CHECK_INTERVAL 100  // ms
#define POWER_LOSS_DELAY 5000     // ms (5 seconds delay before deep sleep)
```

#### 3. esp32/wifi_manager.h

**Add WiFi Reconnection Method**:
```cpp
// Reconnect to WiFi (for wake-up)
int reconnectToWiFi() {
  String ssid = configMgr->getSsid();
  String pass = configMgr->getPassword();
  
  if (ssid.length() == 0 || pass.length() == 0) {
    Serial.println("[WiFiManager] No saved WiFi credentials");
    return -1;
  }
  
  Serial.print("[WiFiManager] Reconnecting to: ");
  Serial.println(ssid);
  
  // Disconnect any existing connection
  if (WiFi.isConnected()) {
    WiFi.disconnect(false);
    delay(100);
  }
  
  // Attempt connection
  WiFi.begin(ssid.c_str(), pass.c_str());
  
  unsigned long startTime = millis();
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Timeout after 10 seconds
    if (millis() - startTime > 10000) {
      Serial.println();
      Serial.println("[WiFiManager] Reconnection timeout");
      WiFi.disconnect(false);
      return -1;
    }
    
    // Fail if specific error
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println();
      Serial.println("[WiFiManager] Reconnection failed");
      WiFi.disconnect(false);
      return -1;
    }
  }
  
  Serial.println();
  Serial.print("[WiFiManager] Reconnected! IP: ");
  Serial.println(WiFi.localIP());
  
  configMgr->setWifiConnected(true);
  return 0;
}
```

### 4. New Command: Power Status

**Add to esp32/esp32.ino**:
```cpp
// Add to commands array
{ 21, "get_power_status", 0, get_power_status, false },

// Add function declaration
void get_power_status();

// Add function implementation
void get_power_status() {
  Serial.println("[CMD] get_power_status");
  
  String statusJson = "{";
  statusJson += "\"powered\":" + String(isPowered ? "true" : "false") + ",";
  statusJson += "\"deepSleep\":" + String(powerLossDetected ? "true" : "false") + ",";
  statusJson += "\"bootCount\":" + String(bootCount) + ",";
  statusJson += "\"wifiConnected\":" + String(wifiConnected ? "true" : "false") + ",";
  statusJson += "\"lastIP\":\"";
  statusJson += lastIP;
  statusJson += "\"";
  statusJson += "}";
  
  strncpy(message, statusJson.c_str(), MAXSTRARGLEN - 1);
  setSuccess(message);
}
```

### 5. Calculator Program Updates

**Add Power Status Display to WIFISCAN**:
```basic
PROGRAM:WIFISCAN
:ClrHome
:Disp "SCANNING WIFI..."
:Send(15)
:GetCalc(Str0)
:ClrHome
:Disp "NETWORKS:"
:Disp Str0
:
:Disp "GETTING IP..."
:Send(20)
:GetCalc(Str1)
:Disp "IP:"
:Disp Str1
:
:Disp "POWER STATUS..."
:Send(21)  // NEW: get_power_status
:GetCalc(Str2)
:Disp Str2  // Shows power status
:Pause
:ClrHome
:Stop
```

## Configuration Options

### Deep Sleep Delay
- **Default**: 5 seconds
- **Configurable**: Via NVS or compile-time constant
- **Purpose**: Prevents false disconnections from triggering sleep

### Wake-up Behavior
- **Immediate**: Wake when power detected
- **Delayed**: Wait for stable power before waking
- **WiFi Reconnect**: Only when needed (after wake-up)

### Power Detection Sensitivity
- **Digital**: Simple HIGH/LOW detection
- **Debouncing**: Filter out noise
- **Calibration**: Adjust for different calculators

## Testing Plan

### Unit Tests
1. **Power Detection**: Verify HIGH/LOW detection works
2. **Deep Sleep**: Verify entry and exit
3. **WiFi Reconnection**: Verify reconnection after wake-up
4. **State Preservation**: Verify RTC memory works

### Integration Tests
1. **Calculator Connection**: Test with actual calculator
2. **Disconnection**: Test power loss detection
3. **Reconnection**: Test wake-up and recovery
4. **OTA Updates**: Test OTA after wake-up

### Power Consumption Tests
1. **Deep Sleep Current**: Measure with multimeter
2. **Wake-up Time**: Measure recovery time
3. **WiFi Reconnection Time**: Measure connection time

## Safety Considerations

### Voltage Protection
- **Existing 1kΩ resistors** provide current limiting
- **GPIO protection** - ESP32 GPIO is 5V tolerant
- **No over-voltage risk** with existing setup

### Power Management
- **Graceful shutdown** - Save state before sleep
- **Clean wake-up** - Reinitialize peripherals
- **Error handling** - Handle failed WiFi reconnection

## Migration Path

### Phase 1: Basic Implementation
- Add power detection using TIP/RING pins
- Implement deep sleep entry
- Test with calculator

### Phase 2: Wake-up Management
- Add wake-up logic
- Implement WiFi reconnection
- Add state preservation

### Phase 3: Optimization
- Add configurable delay
- Add power status reporting
- Add manual control commands

### Phase 4: Documentation
- Update README
- Add hardware diagram
- Add troubleshooting guide

## Expected Behavior

### Normal Operation
1. **Calculator Connected**: ESP32 awake, WiFi connected, OTA server running
2. **Calculator Disconnected**: ESP32 waits 5 seconds, then enters deep sleep
3. **Calculator Reconnected**: ESP32 wakes up, reconnects WiFi, restarts OTA

### Power Consumption
- **Deep Sleep**: ~10µA (months on small LiPo)
- **Active (WiFi connected)**: ~100-200mA
- **Active (WiFi idle)**: ~50-100mA

### Recovery Time
- **Deep Sleep to Wake**: ~100ms
- **WiFi Reconnection**: ~2-5 seconds
- **OTA Server Restart**: ~500ms
- **Total Recovery**: ~3-6 seconds

## Conclusion

This software-only power management solution provides automatic deep sleep/wake functionality using existing TIP/RING pins. It requires no hardware modifications, maintains full backward compatibility, and provides significant power savings when the calculator is disconnected. The implementation is straightforward and can be tested incrementally.