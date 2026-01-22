================================================================================
TI-32 TI-84 Plus WiFi & GPT Mod (ESP32-CAM Edition) - Complete Documentation

OVERVIEW
================================================================================
TI-32 interfaces an AI-Thinker ESP32-CAM board with a TI-84 Plus calculator to
provide internet access, ChatGPT integration, camera features, and wireless app installation.

This guide is specific to the "AI-Thinker ESP32-CAM" module.

================================================================================
⚠️  CRITICAL SAFETY WARNING - READ BEFORE BUILDING

VOLTAGE MISMATCH RISK:
The TI-84 Plus operates at 5V logic.
The ESP32 operates at 3.3V logic.
DO NOT connect the calculator's data lines directly to the ESP32. Doing so will
permanently destroy the ESP32's GPIO pins. You MUST use a Logic Level Converter.

COMMON GROUND:
You MUST connect the grounds of the Calculator, the Level Converter, and
the ESP32 together. If you skip this, the device will not communicate.

================================================================================
1. HARDWARE REQUIREMENTS
================================================================================

Calculator:
TI-84 Plus Silver Edition (Monochrome) OR
TI-84 Plus C Silver Edition (Color)

Microcontroller:
AI-Thinker ESP32-CAM (with OV2640 camera module)

Interface:
SparkFun Logic Level Converter (Bi-Directional)
2.5mm Stereo Cable (to cut open) or 22AWG wire

Power Components:
1x SPDT Slide Switch
1x 1N4001 Diode (REQUIRED for Monochrome TI-84 Plus only)

================================================================================
2. WIRING GUIDE

A. PIN MAPPINGS (AI-Thinker ESP32-CAM)
Signal: TIP  ----------------> GPIO 12
Signal: RING ----------------> GPIO 13

B. DATA WIRING DIAGRAM

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

C. POWER SUPPLY WIRING
[Calc Battery +] --> [Slide Switch] --> [DIODE 1N4001] --> [ESP32-CAM '5V' Pin]
Note:
- The diode is REQUIRED for AAA powered calculators (~6.4V) to drop voltage safely.
- The stripe on the diode must face towards the ESP32.

================================================================================
3. SOFTWARE SETUP: SERVER & NGROK

STEP 1: The Node.js Server
Install Node.js on your computer.
Navigate to your project folder: cd TI-32/server
Install dependencies: npm install
Create a .env file in this folder:
PORT=8080
HTTP_USERNAME="admin"
HTTP_PASSWORD="password123"
OPENROUTER_API_KEY="your_key_here"

Run the server: node index.mjs

STEP 2: Ngrok
Download ngrok.exe from ngrok.com.
Place ngrok.exe into your TI-32/server folder.
Authenticate: .\ngrok config add-authtoken YOUR_TOKEN_HERE
Start tunnel: .\ngrok http 8080
Copy the Forwarding URL (e.g., https://abc-123.ngrok-free.app).

================================================================================
4. SOFTWARE SETUP: ESP32 FIRMWARE

1. Open esp32/esp32.ino in Arduino IDE.
2. Select Board: "AI Thinker ESP32-CAM".
3. Configure Secrets:
   - Rename `esp32/secrets.h.template` to `esp32/secrets.h`.
   - Update with your WiFi and Ngrok details.
4. Upload the code via USB-to-Serial adapter.

================================================================================
5. AP CONFIGURATION PORTAL & OTA

WiFi Failover / Configuration:
If the ESP32 fails to connect to the saved WiFi, it will start an Access Point
named "TI-32-Config". Connect to this WiFi with your computer and go to
http://192.168.4.1 in your browser. You can update WiFi and API settings via
the web form.

OTA Updates:
Once connected to WiFi, you can flash new firmware wirelessly from Arduino IDE
by selecting the "TI-32" network port.

================================================================================
6. INSTALLATION ON CALCULATOR ("Silent Transfer")

1. Press [CLEAR] on calculator home screen.
2. Type '5', press [STO->], press [ALPHA][PRGM] (C), press [ENTER].
3. Press [PRGM] -> [I/O] tab -> '2:Send('.
4. Press [ALPHA][PRGM] (C), press [ENTER].
5. Screen will say "Done", then "Receiving... TI32" after a few seconds.
6. Launch: Press [PRGM], select TI32, press [ENTER].

================================================================================
7. TROUBLESHOOTING

"Error: Link": Check wiring (GPIO 12/13), ensure common ground.
"WiFi Failed": Connect to "TI-32-Config" AP to reconfigure credentials.
"Camera Failed": Ensure board has PSRAM for 720p, or it will fall back to SVGA.
