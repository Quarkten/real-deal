# Power Management Feature Plan

## Overview

Implement automatic deep sleep/wake functionality for ESP32-CAM based on power presence from the calculator connection.

## Requirements

### Hardware Context
- **Connection**: ESP32-CAM connected to TI-84 Plus via 2.5mm jack
  - **Sleeve**: Ground (GND)
  - **Tip**: Data 1 (GPIO 12)
  - **Ring**: Data 2 (GPIO 13)
- **Power**: Calculator provides power through the connection
- **ESP32**: 3.3V logic, requires stable power supply

### Functional Requirements
1. **Wake Up**: ESP32 should wake from deep sleep when calculator provides power
2. **Deep Sleep**: ESP32 should enter deep sleep when power is removed
3. **State Preservation**: Maintain WiFi connection state when possible
4. **Battery Saving**: Minimize power consumption during deep sleep

## Implementation Strategy

### 1. Power Detection Method

**Option A: Voltage Monitoring (Recommended)**
- Use a voltage divider to monitor power presence on TIP/RING
- Connect to ADC-capable GPIO (e.g., GPIO 34, 35, 36, 39)
- Detect voltage drop when calculator disconnects

**Option B: GPIO Wake-up**
- Use existing TIP/RING pins as wake-up sources
- Configure as external wake-up pins
- ESP32 wakes when voltage is applied

**Option C: Power Management IC**
- Use a power management chip to detect power presence
- More complex but more reliable

**Recommended**: Option A (Voltage Monitoring) for simplicity and reliability.

### 2. Hardware Modifications

#### Voltage Divider Circuit
```
Calculator Power (5V) → Voltage Divider → ESP32 ADC Pin
                      ↓
                    GND
```

**Components needed**:
- 10kΩ resistor (R1)
- 20kΩ resistor (R2)
- Capacitor for stability (optional)

**Calculation**:
- Input: 5V from calculator
- Output: 3.3V (safe for ESP32)
- Ratio: R2/(R1+R2) = 20k/(10k+20k) = 0.667
- Output: 5V × 0.667 = 3.33V ✓

#### Connection Diagram
```
Calculator 2.5mm Jack:
  Sleeve (GND) → ESP32 GND
  Tip (Data 1) → ESP32 GPIO 12 (via 1kΩ resistor)
  Ring (Data 2) → ESP32 GPIO 13 (via 1kΩ resistor)
  
Power Detection:
  Tip/Ring → Voltage Divider → ESP32 GPIO 34 (ADC1_CH6)
```

### 3. Firmware Implementation

#### Deep Sleep Configuration

**Wake-up Sources**:
- External wake-up via GPIO 34 (power detection)
- Timer wake-up (optional, for periodic checks)

**Deep Sleep Mode**:
- Disable WiFi radio
- Disable peripherals
- Keep RTC memory for state
- Minimal power consumption (~10µA)

#### Power Detection Logic

```cpp
// Power detection threshold (adjust based on measurements)
#define POWER_THRESHOLD 2.5f  // Volts
#define POWER_PIN 34          // ADC pin for power detection

// State variables
bool isPowered = false;
bool wasPowered = false;
unsigned long lastPowerCheck = 0;
#define POWER_CHECK_INTERVAL 100  // ms

void setupPowerDetection() {
  pinMode(POWER_PIN, INPUT);
  analogReadResolution(12);  // 12-bit resolution (0-4095)
}

float readPowerVoltage() {
  int raw = analogRead(POWER_PIN);
  float voltage = (raw * 3.3f) / 4095.0f;
  // Adjust for voltage divider if needed
  voltage = voltage * (10.0f + 20.0f) / 20.0f;  // R1+R2 / R2
  return voltage;
}

bool checkPowerPresence() {
  float voltage = readPowerVoltage();
  return voltage > POWER_THRESHOLD;
}

void handlePowerState() {
  isPowered = checkPowerPresence();
  
  if (isPowered != wasPowered) {
    if (isPowered) {
      // Power restored - wake up
      Serial.println("[Power] Power restored - waking up");
      wakeUp();
    } else {
      // Power lost - enter deep sleep
      Serial.println("[Power] Power lost - entering deep sleep");
      enterDeepSleep();
    }
    wasPowered = isPowered;
  }
}
```

#### Deep Sleep Implementation

```cpp
#include "esp_sleep.h"

void enterDeepSleep() {
  Serial.println("[Power] Entering deep sleep...");
  
  // Save state to RTC memory if needed
  // esp_sleep_enable_timer_wakeup(TIME_IN_SECONDS * 1000000);
  
  // Configure wake-up on GPIO 34 (power detection)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_PIN, 1);  // Wake on high
  
  // Disable WiFi to save power
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // Enter deep sleep
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
    } else {
      Serial.println("\n[Power] WiFi reconnection failed");
    }
  }
  
  // Restart OTA server
  otaMgr.begin();
}
```

#### Integration with Main Loop

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

### 4. State Management

#### RTC Memory Usage
- Store WiFi credentials in NVS (already implemented)
- Store connection state in RTC memory
- Store last known IP address (optional)

#### State Preservation
```cpp
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool wifiConnected = false;
RTC_DATA_ATTR char lastIP[16] = {0};

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

### 5. Power Consumption Optimization

#### During Deep Sleep
- WiFi radio: OFF
- CPU: OFF
- Peripherals: OFF
- RTC: ON (for wake-up)
- Expected consumption: ~10µA

#### During Active Mode
- WiFi radio: ON (when connected)
- CPU: ON
- Peripherals: ON
- Expected consumption: ~100-200mA (when WiFi active)

### 6. Testing Plan

#### Hardware Testing
1. Measure voltage at power detection pin with calculator connected/disconnected
2. Verify voltage divider output is safe for ESP32 (3.3V max)
3. Test deep sleep current consumption
4. Test wake-up time

#### Software Testing
1. Test power detection accuracy
2. Test deep sleep entry/exit
3. Test WiFi reconnection after wake-up
4. Test OTA functionality after wake-up
5. Test command processing after wake-up

### 7. Safety Considerations

#### Voltage Protection
- Use voltage divider to prevent over-voltage
- Add TVS diode for transient protection
- Ensure 3.3V maximum to ESP32 GPIO

#### Power Management
- Monitor battery level if using battery backup
- Implement graceful shutdown
- Log power events for debugging

#### Calculator Protection
- Ensure no back-powering of calculator
- Use appropriate resistors to limit current
- Follow TI-84 Plus I/O specifications

### 8. Integration with Existing Features

#### WiFi Management
- Existing WiFi connection logic remains unchanged
- Power management adds automatic sleep/wake
- WiFi credentials persist in NVS

#### OTA Updates
- OTA server only runs when powered
- Wake-up automatically restarts OTA server
- No changes to OTA update process

#### Calculator Commands
- All existing commands work when powered
- Commands fail gracefully when in deep sleep
- Power status can be queried via new command

### 9. User Experience

#### Automatic Behavior
- **When calculator connected**: ESP32 wakes up, connects to WiFi, starts OTA server
- **When calculator disconnected**: ESP32 enters deep sleep, saves power
- **When calculator reconnected**: ESP32 wakes up, reconnects to WiFi

#### Manual Control (Optional)
- Add command to disable auto-sleep (for debugging)
- Add command to manually enter deep sleep
- Add command to check power status

### 10. Implementation Steps

1. **Hardware Setup**
   - Add voltage divider circuit
   - Connect to GPIO 34
   - Test voltage readings

2. **Firmware Changes**
   - Add power detection functions
   - Add deep sleep/wake functions
   - Integrate with main loop
   - Add state management

3. **Testing**
   - Unit test power detection
   - Integration test with calculator
   - Power consumption measurements
   - OTA update testing

4. **Documentation**
   - Update README with power management info
   - Add hardware modification guide
   - Add troubleshooting section

### 11. Files to Modify

- [`esp32/esp32.ino`](esp32/esp32.ino) - Add power management logic
- [`esp32/config.h`](esp32/config.h) - Add power management constants
- [`esp32/wifi_manager.h`](esp32/wifi_manager.h) - Add WiFi reconnection after wake-up
- [`esp32/ota_manager.h`](esp32/ota_manager.h) - Ensure OTA server restarts after wake-up

### 12. Potential Issues & Solutions

| Issue | Solution |
|-------|----------|
| False wake-ups | Add debouncing, adjust voltage threshold |
| WiFi reconnection fails | Add retry logic, longer timeout |
| Deep sleep current too high | Check for peripherals still powered |
| Calculator power insufficient | Add external power option |
| Voltage divider inaccurate | Use precision resistors, calibrate |

### 13. Future Enhancements

- **Battery backup**: Add LiPo battery with automatic switching
- **Power monitoring**: Add battery level reporting
- **Sleep scheduling**: Timed wake-up for periodic tasks
- **Low power WiFi**: Use WiFi modem sleep mode
- **Energy harvesting**: Solar or kinetic energy harvesting

## Conclusion

This power management feature will significantly improve battery life when the ESP32-CAM is used with the calculator. The implementation is straightforward using the ESP32's built-in deep sleep capabilities and external wake-up sources. The voltage monitoring approach provides reliable power detection while maintaining compatibility with existing hardware.