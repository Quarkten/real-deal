================================================================================ TI-32 TI-84 Plus WiFi & GPT Mod - Complete Documentation

OVERVIEW

TI-32 interfaces an Adafruit ESP32 Feather board with a TI-84 Plus calculator to
provide internet access, ChatGPT integration, and wireless app installation.

This guide is specific to the "Adafruit HUZZAH32 ESP32 Feather" board.

================================================================================ ⚠️  CRITICAL SAFETY WARNING - READ BEFORE BUILDING

VOLTAGE MISMATCH RISK:

The TI-84 Plus operates at 5V logic.

The ESP32 operates at 3.3V logic.

DO NOT connect the calculator's data lines directly to the ESP32. Doing so will
permanently destroy the ESP32's GPIO pins. You MUST use a Logic Level Converter.

COMMON GROUND:

You MUST connect the grounds of the Calculator, the Level Converter, and
the ESP32 together. If you skip this, the device will not communicate.

================================================================================

HARDWARE REQUIREMENTS
================================================================================

Calculator:

TI-84 Plus Silver Edition (Monochrome) OR

TI-84 Plus C Silver Edition (Color)

Microcontroller:

Adafruit ESP32 Feather (4MB Flash, WiFi + Bluetooth)

Product ID: 3405

Interface:

SparkFun Logic Level Converter (Bi-Directional)

2.5mm Stereo Cable (to cut open) or 22AWG wire

Power Components:

1x SPDT Slide Switch

1x 1N4001 Diode (REQUIRED for Monochrome TI-84 Plus only)

================================================================================ 2. WIRING GUIDE

A. PIN MAPPINGS (Adafruit Feather)

We use pins A0 and A1 because the Feather does not have pins labeled D1/D10.



B. DATA WIRING DIAGRAM

  TI-84 PLUS                   LOGIC CONVERTER                 ESP32 FEATHER


(2.5mm Port)                 (SparkFun BOB-12009)             (Adafruit Huzzah)

[Signal: TIP]  ----------------> [HV1]    [LV1] <-------------- [Pin A0]
[Signal: RING] ----------------> [HV2]    [LV2] <-------------- [Pin A1]

[Power: 5V]    ----------------> [HV]                           [Pin USB]
^
|
[LV] <------------------------ [Pin 3V]
(Reference Voltage)

[Ground]       ----------------> [GND] -- [GND] <-------------- [Pin GND]
|
(Common Ground)

C. POWER SUPPLY WIRING

How to power the ESP32 using the calculator's batteries.



OPTION B: TI-84 Plus Silver Edition (Monochrome)
Battery Type: 4x AAA Batteries (~6.4V when fresh)
Wiring:
[Calc Battery +] --> [Slide Switch] --> [DIODE 1N4001] --> [ESP32 'USB' Pin]
Note:
- The diode is REQUIRED to drop voltage below 6V to protect the regulator.
- The stripe on the diode must face towards the ESP32.

================================================================================ 3. SOFTWARE SETUP: SERVER & NGROK

You need two windows running on your computer for the internet features to work.

STEP 1: The Node.js Server ("The Chef")

Install Node.js on your computer.

Navigate to your project folder: cd TI-32/server

Install dependencies: npm install

Create a .env file in this folder:
PORT=8080
HTTP_USERNAME="admin"
HTTP_PASSWORD="password123"
OPENAI_API_KEY="sk-..."  (Get this from OpenAI)

Run the server: node index.mjs

STEP 2: Ngrok ("The Delivery Driver")
This allows the ESP32 to talk to your computer even if they are on different networks.

Download ngrok.exe from ngrok.com.

Place ngrok.exe directly into your TI-32/server folder.

Open a NEW PowerShell window in that folder.

Authenticate (first time only):
.\ngrok config add-authtoken YOUR_TOKEN_HERE

Start the tunnel:
.\ngrok http 8080

Copy the Forwarding URL (e.g., https://www.google.com/search?q=https://abc-123.ngrok-free.app).

⚠️ CRITICAL WARNING:
Every time you restart Ngrok (if using the free version), the URL CHANGES.
You must update secrets.h on the ESP32 every time you restart Ngrok.

================================================================================ 4. SOFTWARE SETUP: ESP32 FIRMWARE

Open esp32/esp32.ino in Arduino IDE.

EDIT PIN DEFINITIONS (Lines ~35): Change the pin numbers to match the wiring (A0=26, A1=25).

constexpr auto TIP = 26;   // GPIO 26 (A0) constexpr auto RING = 25;  // GPIO 25 (A1)

CONFIGURE SECRETS (esp32/secrets.h): Create this file and paste the following. Replace values with yours.

#ifndef SECRETS_H
#define SECRETS_H

// WiFi Credentials (2.4GHz ONLY)
#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASS "Your_WiFi_Password"

// Server Settings
// Paste the Ngrok URL you copied in Step 2 here
#define SERVER "https://www.google.com/search?q=https://abc-123.ngrok-free.app"

// Login (Must match your .env file)
#define HTTP_USERNAME "admin"
#define HTTP_PASSWORD "password123"
#define CHAT_NAME "CalcUser1"

// REQUIRED: Uncomment this line to enable HTTPS for Ngrok
#define SECURE

#endif

Upload the code to your ESP32 Feather via USB.

================================================================================ 5. INSTALLATION ON CALCULATOR ("Silent Transfer")

You do not need a link cable. The ESP32 installs the app internally.

Turn ON the Calculator and the ESP32 Switch.

Press [CLEAR] to go to a blank home screen.

Type the number '5' and store it in variable 'C':

Press [5]

Press [STO->]

Press [ALPHA] then [PRGM] (to type 'C')

Press [ENTER]

Screen should show: 5

Trigger the transfer command:

Press [PRGM]

Press Right Arrow to select [I/O] tab

Select '2:Send('

Press [ALPHA] then [PRGM] (to type 'C')

Press [ENTER]

Screen should show: Send(C)

Wait:

Screen will say "Done".

Wait 2-3 seconds.

Screen will change to "Receiving... TI32".

Launch:

Press [PRGM], select TI32, press [ENTER].

================================================================================ 6. TROUBLESHOOTING

"Error: Link"

Cause: Wiring is likely crossed or grounds are not connected.

Fix: Swap the A0 and A1 pins in the esp32.ino code (swap 26 and 25) and re-upload.

Fix: Ensure GND of Calculator, Converter, and ESP32 are all connected.

"WiFi Failed" / Stuck on "Connecting..."

Cause: Wrong WiFi credentials or 5GHz network.

Fix: ESP32 only supports 2.4GHz WiFi. Check spelling in secrets.h.

"GPT Error" / No Response

Cause: Server not running or Ngrok tunnel closed.

Fix: Ensure node index.mjs AND .\ngrok http 8080 are both running.

Fix: Ensure the Ngrok URL in secrets.h matches the one currently running in the terminal.

Fix: Ensure #define SECURE is uncommented in secrets.h.

"Ngrok Command Not Found"

Cause: PowerShell doesn't see the file.

Fix: Use .\ngrok instead of just ngrok. Ensure ngrok.exe is in the current folder.

```plaintext
      TI-84 PLUS                      LOGIC LEVEL CONVERTER                    ESP32 FEATHER
   (2.5mm Link Port)                   (SparkFun BOB-12009)                    (WiFi Module)

 +-------------------+               +----------------------+              +-------------------+
 |                   |               |                      |              |                   |
 |    [SIGNAL]       |               |       HV SIDE        |              |                   |
 |   (White/Red)     |               |      (5V Logic)      |              |                   |
 |                   |               |                      |              |                   |
 |       TIP  -------+-------------->| HV1              LV1 |<-------------+------- TX / D1    |
 |                   |               |                      |              |                   |
 |                   |               |                      |              |                   |
 |   (Red/White)     |               |                      |              |                   |
 |       RING -------+-------------->| HV2              LV2 |<-------------+------- D10        |
 |                   |               |                      |              |                   |
 |                   |               |                      |              |                   |
 |     [POWER]       |               |                      |              |                   |
 |   (Calc Bat +)    |               |                      |              |                   |
 |      5V+ ---------+-------------->| HV                   |              |                   |
 |                   |               |                      |              |                   |
 |                   |               |       LV SIDE        |              |                   |
 |                   |               |     (3.3V Logic)     |              |                   |
 |                   |               |                      |              |                   |
 |                   |               |                   LV |<-------------+------- 3V (Out)   |
 |                   |               |                      |              |                   |
 |                   |               |                      |              |                   |
 |    [GROUND]       |               |                      |              |                   |
 |   (Copper/Blk)    |               |                      |              |                   |
 |     SLEEVE -------+------+------->| GND              GND |<------+------+------- GND        |
 +-------------------+      |        +----------------------+       |      +-------------------+
                            |                                       |
                            +---------------------------------------+
                                         (Common Ground)
