# TI-32: TI-84 Plus WiFi & GPT Mod (ESP32-CAM Edition) 🚀

[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Hardware: AI-Thinker ESP32-CAM](https://img.shields.io/badge/Hardware-AI--Thinker%20ESP32--CAM-green.svg)](https://docs.ai-thinker.com/en/esp32-cam)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**TI-32** is a revolutionary hardware mod that brings internet access, ChatGPT integration, camera features, and wireless app installation to your **TI-84 Plus** calculator. By interfacing an AI-Thinker ESP32-CAM, your calculator becomes a powerful connected tool.

---

## ⚠️ CRITICAL SAFETY WARNING

> [!CAUTION]
> **VOLTAGE MISMATCH RISK:**
> The TI-84 Plus operates at **5V logic**, while the ESP32 operates at **3.3V logic**.
> **DO NOT** connect the calculator's data lines directly to the ESP32. You **MUST** use a Logic Level Converter to avoid destroying your ESP32.

> [!IMPORTANT]
> **COMMON GROUND:**
> You **MUST** connect the grounds of the Calculator, the Level Converter, and the ESP32 together. Communication will fail without a common ground reference.

---

## 1. 🛠 Hardware Requirements

### Core Components
*   **Calculator:** TI-84 Plus Silver Edition (Monochrome) OR TI-84 Plus C Silver Edition (Color).
*   **Microcontroller:** [AI-Thinker ESP32-CAM](https://docs.ai-thinker.com/en/esp32-cam) (with OV2640 camera module).
*   **Interface:** SparkFun Logic Level Converter (Bi-Directional).
*   **Link Cable:** 2.5mm Stereo Cable (to cut open) or 22AWG wire.

### Power & Protection
*   1x SPDT Slide Switch.
*   1x 1N4001 Diode (Required for AAA-powered Monochrome TI-84 Plus).

---

## 2. 🔌 Wiring Guide

### Pin Mappings (AI-Thinker ESP32-CAM)

| Signal | Calculator Port | ESP32-CAM Pin |
| :--- | :--- | :--- |
| **TIP** | Data Line 1 | **GPIO 12** |
| **RING** | Data Line 2 | **GPIO 13** |
| **SLEEVE** | Ground | **GND** |

### Data Wiring Diagram
```text
  TI-84 PLUS                   LOGIC CONVERTER                 ESP32-CAM
(2.5mm Port)                 (SparkFun BOB-12009)             (AI-Thinker)

[Signal: TIP]  ----------------> [HV1]    [LV1] <-------------- [GPIO 12]
[Signal: RING] ----------------> [HV2]    [LV2] <-------------- [GPIO 13]

[Power: 5V]    ----------------> [HV]                           [Pin 5V]
                                   ^
                                   |
                                 [LV] <------------------------ [Pin 3V3]
                          (Reference Voltage)

[Ground]       ----------------> [GND] -- [GND] <-------------- [Pin GND]
                                   |
                            (Common Ground)
```

### Power Supply Wiring
**[Calc Battery +]** --> **[Slide Switch]** --> **[DIODE 1N4001]** --> **[ESP32-CAM '5V' Pin]**

*Note: The diode drops voltage safely for AAA-powered calculators (~6.4V). Ensure the stripe faces the ESP32.*

---

## 3. 🚀 Software Setup

### Step 1: The Node.js Server
1.  Install [Node.js](https://nodejs.org/).
2.  `cd TI-32/server`
3.  `npm install`
4.  Create a `.env` file:
    ```env
    PORT=8080
    HTTP_USERNAME="admin"
    HTTP_PASSWORD="password123"
    OPENROUTER_API_KEY="your_openrouter_key"
    ```
5.  `node index.mjs`

### Step 2: Ngrok Tunnel
1.  Download [Ngrok](https://ngrok.com/).
2.  Authenticate: `.\ngrok config add-authtoken YOUR_TOKEN`
3.  Start tunnel: `.\ngrok http 8080`
4.  Copy the **Forwarding URL** (e.g., `https://abc-123.ngrok-free.app`).

### Step 3: ESP32 Firmware
1.  Open `esp32/esp32.ino` in Arduino IDE.
2.  Install dependencies: `WiFi`, `HTTPClient`, `Preferences`, `ArduinoOTA`, `esp_camera`.
3.  Select Board: **"AI Thinker ESP32-CAM"**.
4.  Rename `esp32/secrets.h.template` to `esp32/secrets.h` and update your credentials.
5.  **Upload:**
    *   If you have an **FTDI Adapter**, connect it to the ESP32-CAM and hit upload.
    *   If you **don't** have an FTDI, you can use a **Raspberry Pi Pico** as a programmer (see below).

#### 🛠 Alternative: Flashing via Raspberry Pi Pico
If you don't have an FTDI adapter, you can turn a Raspberry Pi Pico into a USB-to-Serial bridge.

**1. Prepare the Pico:**
Upload this code to your Pico using the Arduino IDE (select "Raspberry Pi Pico" as the board):
```cpp
void setup() {
  Serial.begin(115200);  // USB Serial
  Serial1.begin(115200); // Hardware UART0 (GP0/GP1)
}
void loop() {
  if (Serial.available()) Serial1.write(Serial.read());
  if (Serial1.available()) Serial.write(Serial1.read());
}
```

**2. Wiring (Pico to ESP32-CAM):**
| Pico Pin | ESP32-CAM Pin |
| :--- | :--- |
| **VBUS (5V)** | **5V** |
| **GND** | **GND** |
| **GP0 (TX)** | **U0R (RX)** |
| **GP1 (RX)** | **U0T (TX)** |
| - | **GPIO 0** connected to **GND** (Required for Flash Mode) |

**3. Upload:**
With the Pico connected to your PC and the wiring above complete, select the Pico's COM port in Arduino IDE and upload the `esp32.ino` sketch to the ESP32-CAM. **Note:** You may need to press the 'Reset' button on the ESP32-CAM when the IDE says "Connecting...".

---

## 🌐 AP Configuration Portal & OTA

### WiFi Failover
If the ESP32 cannot connect to your saved WiFi, it will automatically start an Access Point:
*   **SSID:** `TI-32-Config`
*   **Portal URL:** `http://192.168.4.1`

Use this portal to update WiFi credentials or your Ngrok URL without reflashing the device!

### Wireless Updates (OTA)
Once connected to WiFi, you can flash firmware updates wirelessly without ever plugging the ESP32 into your computer again.

#### 🔧 How to use Wireless Updates:
1.  Ensure your **ESP32-CAM** and your **computer** are on the same WiFi network.
2.  Open your project in the **Arduino IDE**.
3.  Go to **Tools** -> **Port**.
4.  Look for a section labeled **"Network Ports"**.
5.  Select **"TI-32 at [IP_ADDRESS]"**.
6.  Click **Upload**. The IDE will now send the code over WiFi!

#### 🔍 How to verify it's working:
*   If you see **"TI-32"** under Network Ports, the wireless bridge is active and ready.
*   If it doesn't appear:
    *   Restart the Arduino IDE.
    *   Ensure your computer's firewall isn't blocking UDP port 3232.
    *   Check the Serial Monitor (if plugged in) to ensure it says "Connected to WiFi".

---

## 📥 Installation on Calculator ("Silent Transfer")

1.  Press `[CLEAR]` on the calculator home screen.
2.  Type `5`, press `[STO->]`, press `[ALPHA][PRGM]` (C), then `[ENTER]`.
3.  Press `[PRGM]` -> `[I/O]` tab -> select `2:Send(`.
4.  Press `[ALPHA][PRGM]` (C), then `[ENTER]`.
5.  Wait for "Done", then "Receiving... TI32" will appear.
6.  **Launch:** Press `[PRGM]`, select `TI32`, and press `[ENTER]`.

---

## ❓ Troubleshooting

*   **Error: Link:** Check your wiring to GPIO 12/13. Ensure common ground is connected.
*   **WiFi Failed:** Connect to the `TI-32-Config` AP to reconfigure settings.
*   **Camera Failed:** HD (720p) requires PSRAM. The system will fall back to SVGA if PSRAM is not detected.

---

## 📜 Legacy Support (Adafruit ESP32 Feather)

<details>
<summary>Click to view documentation for the old Feather board</summary>

The original project used the Adafruit ESP32 Feather (Product ID: 3405). This board **does not support camera features**.

### Original Pin Mapping
*   **TIP:** GPIO 26 (A0)
*   **RING:** GPIO 25 (A1)

### Legacy Wiring
The wiring is similar, but uses the Feather's pinout. Reference the code history for specific implementation details for this board.

</details>

---
*Created with ❤️ by ChromaLock*
