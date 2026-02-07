# Power Management Hardware Diagram

## Current Setup (Existing Connections)

```
TI-84 Plus Calculator 2.5mm Jack
┌─────────────────────────────────────┐
│                                     │
│  Sleeve (Base) ───────────────┐    │
│                                │    │
│  Tip (Data 1) ─────────────┐   │    │
│                            │   │    │
│  Ring (Data 2) ──────────┐ │   │    │
│                          │ │   │    │
└──────────────────────────│─│───│────┘
                           │ │   │
                           │ │   │
                    1kΩ    │ │   │ 1kΩ
                    Resistor│ │   │ Resistor
                           │ │   │
                           ▼ ▼   ▼
                    ┌─────────────────┐
                    │  ESP32-CAM      │
                    │                 │
                    │  GPIO 12 (TIP)  │
                    │  GPIO 13 (RING) │
                    │                 │
                    │  GND            │
                    └─────────────────┘
```

## Power Detection Using Existing Pins

### Concept
Since we're using a software-only solution, we'll detect power presence by monitoring the voltage on the existing TIP/RING pins when the calculator is connected.

### How It Works
1. **When Calculator Connected**: 
   - TIP and RING pins receive voltage from calculator (5V through 1kΩ resistors)
   - ESP32 reads HIGH on these pins
   - ESP32 stays awake and operational

2. **When Calculator Disconnected**:
   - TIP and RING pins go LOW (no voltage)
   - ESP32 detects power loss
   - After delay, ESP32 enters deep sleep

### Hardware Requirements
- **No additional components needed** - uses existing TIP/RING connections
- **Existing 1kΩ resistors** provide current limiting and voltage protection
- **GPIO pins** (12 and 13) already connected to calculator

### Voltage Levels
- **Calculator output**: ~5V (through 1kΩ resistor)
- **ESP32 input**: ~3.3V (safe for ESP32 GPIO)
- **Logic level**: HIGH when connected, LOW when disconnected

## Connection Diagram

```
TI-84 Plus Calculator
┌─────────────────────────────────────────┐
│                                         │
│  2.5mm Jack                             │
│  ┌─────────────────────────────────┐   │
│  │                                 │   │
│  │  Sleeve (GND) ───────────────┐  │   │
│  │                               │  │   │
│  │  Tip (Data 1) ─────────────┐  │  │   │
│  │                             │  │  │   │
│  │  Ring (Data 2) ──────────┐  │  │  │   │
│  │                           │  │  │  │   │
│  └───────────────────────────│──│──│──┘   │
│                              │  │  │      │
└──────────────────────────────│──│──│──────┘
                               │  │  │
                    1kΩ        │  │  │ 1kΩ
                    Resistor   │  │  │ Resistor
                               │  │  │
                               ▼  ▼  ▼
                    ┌─────────────────────────┐
                    │  ESP32-CAM              │
                    │                         │
                    │  GPIO 12 (TIP)          │
                    │  GPIO 13 (RING)         │
                    │                         │
                    │  GND                    │
                    │                         │
                    │  Power Detection:       │
                    │  - Monitor GPIO 12/13   │
                    │  - Detect HIGH/LOW      │
                    │                         │
                    │  Deep Sleep:            │
                    │  - Enter after delay    │
                    │  - Wake on GPIO change  │
                    └─────────────────────────┘
```

## Power Flow

### When Calculator Connected
```
Calculator 5V → 1kΩ Resistor → ESP32 GPIO (3.3V) → ESP32 reads HIGH
                                    ↓
                              ESP32 stays awake
                                    ↓
                              WiFi connected
                                    ↓
                              OTA server running
```

### When Calculator Disconnected
```
No voltage → ESP32 GPIO reads LOW → Power loss detected
                                    ↓
                              Wait for delay (configurable)
                                    ↓
                              Enter deep sleep
                                    ↓
                              WiFi radio OFF
                                    ↓
                              Minimal power consumption (~10µA)
```

## Software-Only Power Detection

### Detection Method
Since we're using existing pins, we'll monitor the TIP/RING pins for voltage presence:

```cpp
// Use existing TIP/RING pins for power detection
#define POWER_PIN_TIP 12
#define POWER_PIN_RING 13

// Power detection threshold (adjust based on measurements)
#define POWER_THRESHOLD 2.0f  // Volts

void setupPowerDetection() {
  // Configure pins as inputs
  pinMode(POWER_PIN_TIP, INPUT);
  pinMode(POWER_PIN_RING, INPUT);
  
  // Enable internal pull-down resistors (optional)
  // This ensures pins read LOW when disconnected
  // digitalWrite(POWER_PIN_TIP, LOW);
  // digitalWrite(POWER_PIN_RING, LOW);
}

bool checkPowerPresence() {
  // Read voltage on both pins
  int tipValue = digitalRead(POWER_PIN_TIP);
  int ringValue = digitalRead(POWER_PIN_RING);
  
  // Power is present if either pin is HIGH
  // (Calculator may only drive one pin at a time)
  return (tipValue == HIGH) || (ringValue == HIGH);
}
```

### Advantages of Software-Only Approach
1. **No hardware modifications** needed
2. **Uses existing connections** to calculator
3. **Simple implementation**
4. **No additional components** required
5. **Maintains existing functionality**

### Limitations
1. **Less precise** than voltage divider method
2. **May have false positives/negatives** due to noise
3. **Requires calibration** for different calculators
4. **May not detect very brief disconnections**

## Power Consumption Estimates

### Deep Sleep Mode
- **WiFi radio**: OFF
- **CPU**: OFF
- **Peripherals**: OFF
- **RTC**: ON (for wake-up)
- **Current**: ~10µA
- **Battery life**: Months on small LiPo

### Active Mode (Connected)
- **WiFi radio**: ON (when connected)
- **CPU**: ON
- **Peripherals**: ON
- **Current**: ~100-200mA (when WiFi active)
- **Battery life**: Hours to days depending on usage

## Implementation Strategy

### Phase 1: Basic Power Detection
1. Monitor TIP/RING pins for voltage
2. Detect power loss
3. Enter deep sleep after delay

### Phase 2: Wake-up Management
1. Wake on GPIO change
2. Reconnect WiFi if needed
3. Restart OTA server

### Phase 3: Optimization
1. Add configurable delay
2. Add power status reporting
3. Add manual control commands

## Safety Considerations

### Voltage Protection
- **Existing 1kΩ resistors** provide current limiting
- **GPIO protection** - ESP32 GPIO is 5V tolerant
- **No over-voltage risk** with existing setup

### Power Management
- **Graceful shutdown** - Save state before sleep
- **Clean wake-up** - Reinitialize peripherals
- **Error handling** - Handle failed WiFi reconnection

## Testing Plan

### Hardware Testing
1. Measure voltage on TIP/RING with calculator connected
2. Verify GPIO reads HIGH when connected
3. Verify GPIO reads LOW when disconnected
4. Test deep sleep current consumption

### Software Testing
1. Test power detection accuracy
2. Test deep sleep entry/exit
3. Test WiFi reconnection after wake-up
4. Test OTA functionality after wake-up

## Conclusion

This software-only solution uses the existing TIP/RING connections to detect power presence. It's simple, requires no hardware modifications, and maintains full compatibility with existing functionality. The ESP32 will automatically enter deep sleep when the calculator is disconnected and wake up when reconnected, providing significant power savings while maintaining the ability to quickly resume operation.