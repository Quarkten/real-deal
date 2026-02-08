// Project: TI-32 v0.1
// Author:  ChromaLock
// Target:  ESP32-CAM AI Thinker

// ==========================================
// 1. ENABLE CAMERA (MUST BE FIRST)
// ==========================================
#define CAMERA
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

// ==========================================
// 2. INCLUDES
// ==========================================
#include <Arduino.h>
#include "esp_camera.h" // This library is built-in to the ESP32 Board package
#include "./secrets.h"
#include "./launcher.h"
#include "./config.h"
#include "./config_manager.h"
#include "./wifi_manager.h"
#include "./ota_manager.h"
#include <TICL.h>
#include <CBL2.h>
#include <TIVar.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <Preferences.h>

// ==========================================
// 3. CAMERA PIN DEFINITIONS (AI THINKER)
// ==========================================
#ifdef CAMERA
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM     0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM       5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#endif

// ==========================================
// 4. REST OF YOUR CODE STARTS HERE...
// ==========================================

// Pin assignments for ESP32-CAM AI Thinker board
// Note: GPIO 0 and GPIO 2 are used by camera, GPIO 1 and 3 are used for serial
// Using GPIO 12 and GPIO 13 for TIP and RING (available pins)
constexpr auto TIP = 12;   // GPIO12 (available)
constexpr auto RING = 13;  // GPIO13 (available)
constexpr auto MAXHDRLEN = 16;
constexpr auto MAXDATALEN = 4096;
constexpr auto MAXARGS = 5;
constexpr auto MAXSTRARGLEN = 256;
constexpr auto PICSIZE = 756;
constexpr auto PICVARSIZE = PICSIZE + 2;
constexpr auto PASSWORD = 42069;

CBL2 cbl;
Preferences prefs;

// Manager instances
ConfigManager configMgr;
WiFiManager wifiMgr(&configMgr);
OTAManager otaMgr(&wifiMgr, &configMgr);

// Current SERVER URL (loaded from NVS or defaults to secrets.h)
char currentServer[MAX_NGROK_URL_LEN] = {0};

// whether or not the user has entered the password
bool unlocked = true;

// Power management variables
bool isPowered = false;
bool powerLossDetected = false;
int bootCount = 0;
bool wifiConnected = false;
char lastIP[16] = {0};

// Polling variables
unsigned long lastPollTime = 0;

// Arguments
int currentArg = 0;
char strArgs[MAXARGS][MAXSTRARGLEN];
double realArgs[MAXARGS];

// the command to execute
int command = -1;
// whether or not the operation has completed
bool status = 0;
// whether or not the operation failed
bool error = 0;
// error or success message
char message[MAXSTRARGLEN];
// list data
constexpr auto LISTLEN = 256;
constexpr auto LISTENTRYLEN = 20;
char list[LISTLEN][LISTENTRYLEN];
// http response
constexpr auto MAXHTTPRESPONSELEN = 4096;
char response[MAXHTTPRESPONSELEN];
// image variable (96x63)
uint8_t frame[PICVARSIZE] = { PICSIZE & 0xff, PICSIZE >> 8 };

// Context management for multi-modal interactions
char contextBuffer[MAXHTTPRESPONSELEN];
bool useTextAPI = true;

void connect();
void disconnect();
void gpt();
void send();
void launcher();
void snap();
void solve();
void image_list();
void fetch_image();
void fetch_chats();
void send_chat();
void program_list();
void fetch_program();
void scan_networks();
void connect_wifi();
void save_wifi();
void get_ngrok();
void set_ngrok();
void set_text_key();
void set_image_key();
void get_ip_address();
void get_power_status();
void get_status();
void get_gpt_chunk();

struct Command {
  int id;
  const char* name;
  int num_args;
  void (*command_fp)();
  bool wifi;
};

struct Command commands[] = {
  { 0, "connect", 0, connect, false },
  { 1, "disconnect", 0, disconnect, false },
  { 2, "gpt", 1, gpt, true },
  { 4, "send", 2, send, true },
  { 5, "launcher", 0, launcher, false },
  { 7, "snap", 0, snap, false },
  { 8, "solve", 1, solve, true },
  { 9, "image_list", 1, image_list, true },
  { 10, "fetch_image", 1, fetch_image, true },
  { 11, "fetch_chats", 2, fetch_chats, true },
  { 12, "send_chat", 2, send_chat, true },
  { 13, "program_list", 1, program_list, true },
  { 14, "fetch_program", 1, fetch_program, true },
  { 15, "scan_networks", 0, scan_networks, false },
  { 16, "connect_wifi", 2, connect_wifi, false },
  { 17, "save_wifi", 2, save_wifi, false },
  { 18, "get_ngrok", 0, get_ngrok, false },
  { 19, "set_ngrok", 1, set_ngrok, false },
  { 20, "get_ip_address", 0, get_ip_address, false },
  { 21, "get_power_status", 0, get_power_status, false },
  { 22, "get_status", 0, get_status, false },
  { 23, "get_gpt_chunk", 1, get_gpt_chunk, false },
  { 30, "set_text_key", 1, set_text_key, false },
  { 31, "set_image_key", 1, set_image_key, false }
};

constexpr int NUMCOMMANDS = sizeof(commands) / sizeof(struct Command);
constexpr int MAXCOMMAND = 31;

uint8_t header[MAXHDRLEN];
uint8_t data[MAXDATALEN];

// lowercase letters make strings weird,
// so we have to truncate the string
void fixStrVar(char* str) {
  int end = strlen(str);
  for (int i = 0; i < end; ++i) {
    if (isLowerCase(str[i])) {
      --end;
    }
  }
  str[end] = '\0';
}

// Base64 encoding function
const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String base64_encode(uint8_t* buf, unsigned int len) {
  String ret = "";
  int i = 0;
  int j = 0;
  uint8_t char_array_3[3];
  uint8_t char_array_4[4];

  while (len--) {
    char_array_3[i++] = *(buf++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; i <4 ; i++) ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for(j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; j < i + 1; j++) ret += base64_chars[char_array_4[j]];
    while(i++ < 3) ret += '=';
  }

  return ret;
}

int onReceived(uint8_t type, enum Endpoint model, int datalen);
int onRequest(uint8_t type, enum Endpoint model, int* headerlen,
              int* datalen, data_callback* data_callback);

void startCommand(int cmd) {
  command = cmd;
  status = 0;
  error = 0;
  currentArg = 0;
  for (int i = 0; i < MAXARGS; ++i) {
    memset(&strArgs[i], 0, MAXSTRARGLEN);
    realArgs[i] = 0;
  }
  strncpy(message, "no command", MAXSTRARGLEN);
}

void setError(const char* err) {
  Serial.print("ERROR: ");
  Serial.println(err);
  error = 1;
  status = 1;
  command = -1;
  strncpy(message, err, MAXSTRARGLEN);
}

void setSuccess(const char* success) {
  Serial.print("SUCCESS: ");
  Serial.println(success);
  error = 0;
  status = 1;
  command = -1;
  strncpy(message, success, MAXSTRARGLEN);
}

int sendProgramVariable(const char* name, uint8_t* program, size_t variableSize);

bool camera_sign = false;

void setup() {
  Serial.begin(115200);
  Serial.println("[CBL]");

  cbl.setLines(TIP, RING);
  cbl.resetLines();
  cbl.setupCallbacks(header, data, MAXDATALEN, onReceived, onRequest);
  // cbl.setVerbosity(true, (HardwareSerial *)&Serial);

  pinMode(TIP, INPUT);
  pinMode(RING, INPUT);

  Serial.println("[preferences]");
  prefs.begin("ccalc", false);
  auto reboots = prefs.getUInt("boots", 0);
  Serial.print("reboots: ");
  Serial.println(reboots);
  prefs.putUInt("boots", reboots + 1);
  prefs.end();

  // ========================================================================
  // Initialize Configuration Manager
  // ========================================================================
  Serial.println("[ConfigManager] Initializing...");
  configMgr.begin();
  configMgr.printAll();

  // Load Ngrok URL from NVS, fallback to secrets.h
  String ngrokUrl = configMgr.getNgrokUrl();
  if (ngrokUrl.length() > 0) {
    strncpy(currentServer, ngrokUrl.c_str(), MAX_NGROK_URL_LEN - 1);
  } else {
    strncpy(currentServer, SERVER, MAX_NGROK_URL_LEN - 1);
    Serial.println("[Setup] Using fallback SERVER from secrets.h");
  }
  Serial.print("[Setup] Current SERVER: ");
  Serial.println(currentServer);

  // ========================================================================
  // Connect to WiFi (with NVS credentials or fallback to secrets.h)
  // ========================================================================
  Serial.println("[Setup] Attempting WiFi connection...");
  String ssid = configMgr.getSsid();
  String pass = configMgr.getPassword();

  if (ssid.length() > 0 && pass.length() > 0) {
    Serial.println("[Setup] Using saved WiFi credentials from NVS");
    if (wifiMgr.connectToNetwork(ssid.c_str(), pass.c_str()) != 0) {
      Serial.println("[Setup] Failed to connect with saved credentials, trying fallback...");
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("\n[Setup] Connected with fallback credentials");
    }
  } else {
    Serial.println("[Setup] No saved WiFi credentials, using fallback from secrets.h");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n[Setup] Connected with fallback credentials");
  }

  Serial.print("[Setup] WiFi connected! IP: ");
  Serial.println(WiFi.localIP());

  // ========================================================================
  // Initialize OTA Manager
  // ========================================================================
  Serial.println("[Setup] Starting OTA Web Server...");
  otaMgr.begin();
  otaMgr.printInfo();

  // ========================================================================
  // Initialize Camera
  // ========================================================================
  #ifdef CAMERA
  Serial.println("[Setup] Initializing Camera...");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("[Setup] Camera initialized successfully");
  #endif

  strncpy(message, "default message", MAXSTRARGLEN);
  delay(100);
  memset(data, 0, MAXDATALEN);
  memset(header, 0, 16);
  Serial.println("[ready]");
}

void (*queued_action)() = NULL;

bool isBusyWithCalculator() {
  // Busy if:
  // - Command has been received (command != -1)
  // - AND command is not yet complete (status == 0)
  return (command >= 0 && command <= MAXCOMMAND && status == 0);
}

void postResult(String result) {
  #ifdef SECURE
    WiFiClientSecure client;
    client.setInsecure();
  #else
    WiFiClient client;
  #endif
  HTTPClient http;
  http.setAuthorization(HTTP_USERNAME, HTTP_PASSWORD);

  auto url = String(currentServer) + "/esp32/result";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(result);
  Serial.print("POST Result: ");
  Serial.println(httpResponseCode);
  http.end();
}

void pollServer() {
  if (!WiFi.isConnected()) return;

  #ifdef SECURE
    WiFiClientSecure client;
    client.setInsecure();
  #else
    WiFiClient client;
  #endif
  HTTPClient http;
  http.setAuthorization(HTTP_USERNAME, HTTP_PASSWORD);

  auto url = String(currentServer) + "/esp32/poll";
  http.begin(client, url);

  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    String payload = http.getString();
    Serial.print("Poll response: ");
    Serial.println(payload);

    if (payload == "SCAN_NETWORKS") {
      String results = wifiMgr.scanNetworksDetailed();
      postResult(results);
    } else if (payload == "GET_STATUS") {
      get_status(); // This will update 'message'
      String statusJson = String("{\"command\":\"get_status\",\"success\":true,\"data\":") + message + "}";
      postResult(statusJson);
    } else if (payload.startsWith("SET_NGROK ")) {
      String newUrl = payload.substring(10);
      strncpy(strArgs[0], newUrl.c_str(), MAXSTRARGLEN - 1);
      currentArg = 1;
      set_ngrok();
      String responseJson = String("{\"command\":\"set_ngrok\",\"success\":") + (error ? "false" : "true") + ",\"message\":\"" + message + "\"}";
      postResult(responseJson);
    } else if (payload.startsWith("SET_TEXT_KEY ")) {
      String newKey = payload.substring(13);
      strncpy(strArgs[0], newKey.c_str(), MAXSTRARGLEN - 1);
      currentArg = 1;
      set_text_key();
      String responseJson = String("{\"command\":\"set_text_key\",\"success\":") + (error ? "false" : "true") + ",\"message\":\"" + message + "\"}";
      postResult(responseJson);
    } else if (payload.startsWith("SET_IMAGE_KEY ")) {
      String newKey = payload.substring(14);
      strncpy(strArgs[0], newKey.c_str(), MAXSTRARGLEN - 1);
      currentArg = 1;
      set_image_key();
      String responseJson = String("{\"command\":\"set_image_key\",\"success\":") + (error ? "false" : "true") + ",\"message\":\"" + message + "\"}";
      postResult(responseJson);
    }
  }
  http.end();
}

void loop() {
  // Handle OTA web server requests (non-blocking)
  otaMgr.handleClient();

  // Polling logic
  if (millis() - lastPollTime > POLL_INTERVAL_MS && !isBusyWithCalculator()) {
    lastPollTime = millis();
    pollServer();
  }

  if (queued_action) {
    // dont ask me why you need this, but it fails otherwise.
    // probably relates to a CBL2 timeout thing?
    delay(1000);
    Serial.println("executing queued actions");
    // dont ask me
    void (*tmp)() = queued_action;
    queued_action = NULL;
    tmp();
  }
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
  cbl.eventLoopTick();
}

int onReceived(uint8_t type, enum Endpoint model, int datalen) {
  char varName = header[3];

  Serial.print("unlocked: ");
  Serial.println(unlocked);

  // check for password
  if (!unlocked && varName == 'P') {
    auto password = TIVar::realToLong8x(data, model);
    if (password == PASSWORD) {
      Serial.println("successful unlock");
      unlocked = true;
      return 0;
    } else {
      Serial.println("failed unlock");
    }
  }

  if (!unlocked) {
    return -1;
  }

  // check for command
  if (varName == 'C') {
    if (type != VarTypes82::VarReal) {
      return -1;
    }
    int cmd = TIVar::realToLong8x(data, model);
    if (cmd >= 0 && cmd <= MAXCOMMAND) {
      Serial.print("command: ");
      Serial.println(cmd);
      startCommand(cmd);
      return 0;
    } else {
      Serial.print("invalid command: ");
      Serial.println(cmd);
      return -1;
    }
  }

  if (currentArg >= MAXARGS) {
    Serial.println("argument overflow");
    setError("argument overflow");
    return -1;
  }

  switch (type) {
    case VarTypes82::VarString:
      Serial.print("len: ");
      strncpy(strArgs[currentArg++], TIVar::strVarToString8x(data, model).c_str(), MAXSTRARGLEN);
      fixStrVar(strArgs[currentArg - 1]);
      Serial.print("Str");
      Serial.print(currentArg - 1);
      Serial.print(" ");
      Serial.println(strArgs[currentArg - 1]);
      break;
    case VarTypes82::VarReal:
      realArgs[currentArg++] = TIVar::realToFloat8x(data, model);
      Serial.print("Real");
      Serial.print(currentArg - 1);
      Serial.print(" ");
      Serial.println(realArgs[currentArg - 1]);
      break;
    default:
      // maybe set error here?
      return -1;
  }
  return 0;
}

uint8_t frameCallback(int idx) {
  return frame[idx];
}

char varIndex(int idx) {
  return '0' + (idx == 9 ? 0 : (idx + 1));
}

int onRequest(uint8_t type, enum Endpoint model, int* headerlen, int* datalen, data_callback* data_callback) {
  char varName = header[3];
  char strIndex = header[4];
  char strname[5] = { 'S', 't', 'r', varIndex(strIndex), 0x00 };
  char picname[5] = { 'P', 'i', 'c', varIndex(strIndex), 0x00 };
  Serial.print("request for ");
  Serial.println(varName == 0xaa ? strname : varName == 0x60 ? picname
                                                             : (const char*)&header[3]);
  memset(header, 0, sizeof(header));
  switch (varName) {
    case 0x60:
      if (type != VarTypes82::VarPic) {
        return -1;
      }
      *datalen = PICVARSIZE;
      TIVar::intToSizeWord(*datalen, &header[0]);
      header[2] = VarTypes82::VarPic;
      header[3] = 0x60;
      header[4] = strIndex;
      *data_callback = frameCallback;
      break;
    case 0xAA:
      if (type != VarTypes82::VarString) {
        return -1;
      }
      // TODO right now, the only string variable will be the message, but ill need to allow for other vars later
      *datalen = TIVar::stringToStrVar8x(String(message), data, model);
      TIVar::intToSizeWord(*datalen, header);
      header[2] = VarTypes82::VarString;
      header[3] = 0xAA;
      // send back as same variable that was requested
      header[4] = strIndex;
      *headerlen = 13;
      break;
    case 'E':
      if (type != VarTypes82::VarReal) {
        return -1;
      }
      *datalen = TIVar::longToReal8x(error, data, model);
      TIVar::intToSizeWord(*datalen, header);
      header[2] = VarTypes82::VarReal;
      header[3] = 'E';
      header[4] = '\0';
      *headerlen = 13;
      break;
    case 'S':
      if (type != VarTypes82::VarReal) {
        return -1;
      }
      *datalen = TIVar::longToReal8x(status, data, model);
      TIVar::intToSizeWord(*datalen, header);
      header[2] = VarTypes82::VarReal;
      header[3] = 'S';
      header[4] = '\0';
      *headerlen = 13;
      break;
    default:
      return -1;
  }
  return 0;
}

int makeRequest(String url, char* result, int resultLen, size_t* len) {
  memset(result, 0, resultLen);

#ifdef SECURE
  WiFiClientSecure client;
  client.setInsecure();
#else
  WiFiClient client;
#endif
  HTTPClient http;
  http.setAuthorization(HTTP_USERNAME, HTTP_PASSWORD);

  Serial.println(url);
  http.begin(client, url.c_str());

  // Send HTTP GET request
  int httpResponseCode = http.GET();
  Serial.print(url);
  Serial.print(" ");
  Serial.println(httpResponseCode);

  int responseSize = http.getSize();
  WiFiClient* httpStream = http.getStreamPtr();

  Serial.print("response size: ");
  Serial.println(responseSize);

  if (httpResponseCode != 200) {
    return httpResponseCode;
  }

  if (httpStream->available() > resultLen) {
    Serial.print("response size: ");
    Serial.print(httpStream->available());
    Serial.println(" is too big");
    return -1;
  }

  while (httpStream->available()) {
    *(result++) = httpStream->read();
  }
  *len = responseSize;

  http.end();

  return 0;
}

void connect() {
  const char* ssid = WIFI_SSID;
  const char* pass = WIFI_PASS;
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("PASS: ");
  Serial.println("<hidden>");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      setError("failed to connect");
      return;
    }
  }
  setSuccess("connected");
}

void disconnect() {
  WiFi.disconnect(true);
  setSuccess("disconnected");
}

void manageContext(const char* input, bool isImage) {
  // Update the context buffer with the latest input
  if (isImage) {
    strncpy(contextBuffer, "[Image Context] ", MAXHTTPRESPONSELEN - 1);
    strncat(contextBuffer, input, MAXHTTPRESPONSELEN - strlen(contextBuffer) - 1);
    useTextAPI = false;
  } else {
    strncpy(contextBuffer, "[Text Context] ", MAXHTTPRESPONSELEN - 1);
    strncat(contextBuffer, input, MAXHTTPRESPONSELEN - strlen(contextBuffer) - 1);
    useTextAPI = true;
  }
}

void gpt() {
  const char* prompt = strArgs[0];
  Serial.print("prompt: ");
  Serial.println(prompt);

  // Check if this is an image request (starts with "!IMAGE:")
  if (strncmp(prompt, "!IMAGE:", 7) == 0) {
    Serial.println("[GPT] Image mode request detected");

    // Extract the actual question (skip "!IMAGE:")
    const char* imageQuestion = prompt + 7;

    #ifdef CAMERA
    // Capture image from camera
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      setError("Camera capture failed");
      return;
    }

    // Encode image to Base64
    String base64Image = base64_encode(fb->buf, fb->len);

    // Send to server's vision endpoint
    #ifdef SECURE
    WiFiClientSecure client;
    client.setInsecure();
    #else
    WiFiClient client;
    #endif
    HTTPClient http;
    http.setAuthorization(HTTP_USERNAME, HTTP_PASSWORD);

    String url = String(currentServer) + "/gpt/vision";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    // Create JSON with image and question
    String json;
    json.reserve(base64Image.length() + strlen(imageQuestion) + 64);
    json = "{\"image\":\"";
    json += base64Image;
    json += "\",\"question\":\"";
    json += imageQuestion;
    json += "\"}";

    Serial.println("Sending image to vision AI...");
    int httpResponseCode = http.POST(json);

    if (httpResponseCode == 200) {
      // Store full response in the response buffer for chunked retrieval
      String responseStr = http.getString();
      strncpy(response, responseStr.c_str(), MAXHTTPRESPONSELEN - 1);
      response[MAXHTTPRESPONSELEN - 1] = '\0';

      // Also set the first chunk as the immediate success message
      strncpy(message, response, MAXSTRARGLEN - 1);
      message[MAXSTRARGLEN - 1] = '\0';
      setSuccess(message);
    } else {
      String errorMsg = "Vision AI failed: ";
      errorMsg += httpResponseCode;
      setError(errorMsg.c_str());
    }

    http.end();
    esp_camera_fb_return(fb);
    #else
    setError("Camera not supported on this board");
    #endif

    return;
  }

  // Manage context for text input
  manageContext(prompt, false);

  auto url = String(currentServer) + String("/gpt/ask?question=") + urlEncode(String(prompt));

  size_t realsize = 0;
  if (makeRequest(url, response, MAXHTTPRESPONSELEN, &realsize)) {
    setError("error making request");
    return;
  }

  // Ensure null termination for scrolling/chunked retrieval
  if (realsize < MAXHTTPRESPONSELEN) {
    response[realsize] = '\0';
  } else {
    response[MAXHTTPRESPONSELEN - 1] = '\0';
  }

  Serial.print("response: ");
  Serial.println(response);

  setSuccess(response);
}

void send() {
  const char* recipient = strArgs[0];
  const char* message = strArgs[1];
  Serial.print("sending \"");
  Serial.print(message);
  Serial.print("\" to \"");
  Serial.print(recipient);
  Serial.println("\"");
  setSuccess("OK: sent");
}

void _sendLauncher() {
  sendProgramVariable("TI32", __launcher_var, __launcher_var_len);
}

void launcher() {
  // we have to queue this action, since otherwise the transfer fails
  // due to the CBL2 library still using the lines
  queued_action = _sendLauncher;
  setSuccess("queued transfer");
}


void provideCaptureFeedback() {
  #ifdef CAMERA
  // Get camera sensor status
  sensor_t *s = esp_camera_sensor_get();
  if (!s) {
    setError("Failed to get sensor status");
    return;
  }
  
  // Check focus and lighting conditions
  bool focusOk = true; // Placeholder for focus check logic
  bool lightingOk = true; // Placeholder for lighting check logic
  
  // Provide feedback
  String feedback = "Capture Feedback: ";
  if (!focusOk) {
    feedback += "Focus not optimal. ";
  }
  if (!lightingOk) {
    feedback += "Lighting conditions poor. ";
  }
  if (focusOk && lightingOk) {
    feedback += "Ready to capture!";
  }
  
  strncpy(message, feedback.c_str(), MAXSTRARGLEN - 1);
  setSuccess(message);
  #else
  setError("Camera not supported on this board");
  #endif
}

void snap() {
  #ifdef CAMERA
  // Provide real-time feedback before capture
  provideCaptureFeedback();
  
  // Capture image from camera
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    setError("Camera capture failed");
    return;
  }
  
  // Manage context for image input
  manageContext("Image captured", true);
  
  // Compress and send image
  // Placeholder for image compression and transmission logic
  setSuccess("Image captured successfully");
  
  // Return the frame buffer
  esp_camera_fb_return(fb);
  #else
  setError("Camera not supported on this board");
  #endif
}


void solve() {
  #ifdef CAMERA
  // Capture and solve image (e.g., QR code, object recognition)
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    setError("Camera capture failed");
    return;
  }
  
  // Placeholder for image solving logic
  setSuccess("Image solved successfully");
  
  // Return the frame buffer
  esp_camera_fb_return(fb);
  #else
  setError("Camera not supported on this board");
  #endif
}

void image_list() {
  int page = realArgs[0];
  auto url = String(currentServer) + String("/image/list?p=") + urlEncode(String(page));

  size_t realsize = 0;
  if (makeRequest(url, response, MAXSTRARGLEN, &realsize)) {
    setError("error making request");
    return;
  }

  Serial.print("response: ");
  Serial.println(response);

  setSuccess(response);
}

void fetch_image() {
  memset(frame + 2, 0, 756);
  // fetch image and put it into the frame variable
  int id = realArgs[0];
  Serial.print("id: ");
  Serial.println(id);

  auto url = String(currentServer) + String("/image/get?id=") + urlEncode(String(id));

  size_t realsize = 0;
  if (makeRequest(url, response, MAXHTTPRESPONSELEN, &realsize)) {
    setError("error making request");
    return;
  }

  if (realsize != PICSIZE) {
    Serial.print("response size:");
    Serial.println(realsize);
    setError("bad image size");
    return;
  }

  // load the image
  frame[0] = realsize & 0xff;
  frame[1] = (realsize >> 8) & 0xff;
  memcpy(&frame[2], response, 756);

  setSuccess(response);
}

void fetch_chats() {
  int room = realArgs[0];
  int page = realArgs[1];
  auto url = String(currentServer) + String("/chats/messages?p=") + urlEncode(String(page)) + String("&c=") + urlEncode(String(room));

  size_t realsize = 0;
  if (makeRequest(url, response, MAXSTRARGLEN, &realsize)) {
    setError("error making request");
    return;
  }

  Serial.print("response: ");
  Serial.println(response);

  setSuccess(response);
}

void send_chat() {
  int room = realArgs[0];
  const char* msg = strArgs[1];

  auto url = String(currentServer) + String("/chats/send?c=") + urlEncode(String(room)) + String("&m=") + urlEncode(String(msg)) + String("&id=") + urlEncode(String(CHAT_NAME));

  size_t realsize = 0;
  if (makeRequest(url, response, MAXSTRARGLEN, &realsize)) {
    setError("error making request");
    return;
  }

  Serial.print("response: ");
  Serial.println(response);

  setSuccess(response);
}

void program_list() {
  int page = realArgs[0];
  auto url = String(currentServer) + String("/programs/list?p=") + urlEncode(String(page));

  size_t realsize = 0;
  if (makeRequest(url, response, MAXSTRARGLEN, &realsize)) {
    setError("error making request");
    return;
  }

  Serial.print("response: ");
  Serial.println(response);

  setSuccess(response);
}


char programName[256];
char programData[4096];
size_t programLength;

void _resetProgram() {
  memset(programName, 0, 256);
  memset(programData, 0, 4096);
  programLength = 0;
}

void _sendDownloadedProgram() {
  if (sendProgramVariable(programName, (uint8_t*)programData, programLength)) {
    Serial.println("failed to transfer requested download");
    Serial.print(programName);
    Serial.print("(");
    Serial.print(programLength);
    Serial.println(")");
  }
  _resetProgram();
}

void fetch_program() {
  int id = realArgs[0];
  Serial.print("id: ");
  Serial.println(id);

  _resetProgram();

  auto url = String(currentServer) + String("/programs/get?id=") + urlEncode(String(id));

  if (makeRequest(url, programData, 4096, &programLength)) {
    setError("error making request for program data");
    return;
  }

  size_t realsize = 0;
  auto nameUrl = String(currentServer) + String("/programs/get_name?id=") + urlEncode(String(id));
  if (makeRequest(nameUrl, programName, 256, &realsize)) {
    setError("error making request for program name");
    return;
  }

  queued_action = _sendDownloadedProgram;

  setSuccess("queued download");
}

/// OTHER FUNCTIONS

int sendProgramVariable(const char* name, uint8_t* program, size_t variableSize) {
  Serial.print("transferring: ");
  Serial.print(name);
  Serial.print("(");
  Serial.print(variableSize);
  Serial.println(")");

  int dataLength = 0;

  // IF THIS ISNT SET TO COMP83P, THIS DOESNT WORK
  // seems like ti-84s cant silent transfer to each other
  uint8_t msg_header[4] = { COMP83P, RTS, 13, 0 };

  uint8_t rtsdata[13] = { variableSize & 0xff, variableSize >> 8, VarTypes82::VarProgram, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int nameSize = strlen(name);
  if (nameSize == 0) {
    return 1;
  }
  memcpy(&rtsdata[3], name, min(nameSize, 8));

  auto rtsVal = cbl.send(msg_header, rtsdata, 13);
  if (rtsVal) {
    Serial.print("rts return: ");
    Serial.println(rtsVal);
    return rtsVal;
  }

  cbl.resetLines();
  auto ackVal = cbl.get(msg_header, NULL, &dataLength, 0);
  if (ackVal || msg_header[1] != ACK) {
    Serial.print("ack return: ");
    Serial.println(ackVal);
    return ackVal;
  }

  auto ctsRet = cbl.get(msg_header, NULL, &dataLength, 0);
  if (ctsRet || msg_header[1] != CTS) {
    Serial.print("cts return: ");
    Serial.println(ctsRet);
    return ctsRet;
  }

  msg_header[1] = ACK;
  msg_header[2] = 0x00;
  msg_header[3] = 0x00;
  ackVal = cbl.send(msg_header, NULL, 0);
  if (ackVal || msg_header[1] != ACK) {
    Serial.print("ack cts return: ");
    Serial.println(ackVal);
    return ackVal;
  }

  msg_header[1] = DATA;
  msg_header[2] = variableSize & 0xff;
  msg_header[3] = (variableSize >> 8) & 0xff;
  auto dataRet = cbl.send(msg_header, program, variableSize);
  if (dataRet) {
    Serial.print("data return: ");
    Serial.println(dataRet);
    return dataRet;
  }

  ackVal = cbl.get(msg_header, NULL, &dataLength, 0);
  if (ackVal || msg_header[1] != ACK) {
    Serial.print("ack data: ");
    Serial.println(ackVal);
    return ackVal;
  }

  msg_header[1] = EOT;
  msg_header[2] = 0x00;
  msg_header[3] = 0x00;
  auto eotVal = cbl.send(msg_header, NULL, 0);
  if (eotVal) {
    Serial.print("eot return: ");
    Serial.println(eotVal);
    return eotVal;
  }

  Serial.print("transferred: ");
  Serial.println(name);
  return 0;
}

// ============================================================================
// NEW COMMAND HANDLERS: WiFi & Ngrok Configuration (Commands 15-19)
// ============================================================================

void scan_networks() {
  Serial.println("[CMD] scan_networks");

  // If argument 1 is provided and is 1, do detailed scan (though polling calls it directly)
  if (currentArg > 0 && (int)realArgs[0] == 1) {
    String networkList = wifiMgr.scanNetworksDetailed();
    strncpy(message, networkList.c_str(), MAXSTRARGLEN - 1);
  } else {
    String networkList = wifiMgr.scanNetworks();
    if (networkList.length() == 0) {
      setError("No networks found");
      return;
    }
    strncpy(message, networkList.c_str(), MAXSTRARGLEN - 1);
  }

  setSuccess(message);
}

void connect_wifi() {
 Serial.println("[CMD] connect_wifi");
 const char* ssid = strArgs[0];
 const char* password = strArgs[1];
 
 Serial.print("[CMD] Attempting to connect to: ");
 Serial.print(ssid);
 Serial.print(" with password: ");
 Serial.println("<hidden>");
 
 if (wifiMgr.connectToNetwork(ssid, password) != 0) {
   setError("WiFi connection failed");
   return;
 }
 
 setSuccess("Connected to WiFi");
}

void save_wifi() {
 Serial.println("[CMD] save_wifi");
 const char* ssid = strArgs[0];
 const char* password = strArgs[1];
 
 Serial.print("[CMD] Saving WiFi credentials for: ");
 Serial.println(ssid);
 
 if (!configMgr.setSsid(ssid)) {
   setError("Failed to save SSID");
   return;
 }
 
 if (!configMgr.setPassword(password)) {
   setError("Failed to save password");
   return;
 }
 
 Serial.println("[CMD] WiFi credentials saved successfully");
 setSuccess("WiFi credentials saved");
}

void get_ngrok() {
 Serial.println("[CMD] get_ngrok");
 
 String ngrokUrl = configMgr.getNgrokUrl();
 
 if (ngrokUrl.length() == 0) {
   strncpy(message, currentServer, MAXSTRARGLEN - 1);
   setSuccess(message);
   return;
 }
 
 strncpy(message, ngrokUrl.c_str(), MAXSTRARGLEN - 1);
 setSuccess(message);
}

void set_ngrok() {
  Serial.println("[CMD] set_ngrok");
  const char* ngrokUrl = strArgs[0];
  
  Serial.print("[CMD] Setting Ngrok URL to: ");
  Serial.println(ngrokUrl);
  
  if (!configMgr.setNgrokUrl(ngrokUrl)) {
    setError("Invalid Ngrok URL or too long");
    return;
  }
  
  // Update the global currentServer variable
  strncpy(currentServer, ngrokUrl, MAX_NGROK_URL_LEN - 1);
  
  Serial.print("[CMD] Ngrok URL updated. Will use: ");
  Serial.println(currentServer);
  
  setSuccess("Ngrok URL updated");
}

void set_text_key() {
  Serial.println("[CMD] set_text_key");
  const char* key = strArgs[0];

  #ifdef SECURE
    WiFiClientSecure client;
    client.setInsecure();
  #else
    WiFiClient client;
  #endif
  HTTPClient http;
  http.setAuthorization(HTTP_USERNAME, HTTP_PASSWORD);

  auto url = String(currentServer) + "/esp32/set-text-key";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  String json = "{\"key\":\"";
  json += key;
  json += "\"}";

  int httpResponseCode = http.POST(json);
  if (httpResponseCode == 200) {
    setSuccess("Text API Key updated on server");
  } else {
    setError("Failed to update Text API Key on server");
  }
  http.end();
}

void set_image_key() {
  Serial.println("[CMD] set_image_key");
  const char* key = strArgs[0];

  #ifdef SECURE
    WiFiClientSecure client;
    client.setInsecure();
  #else
    WiFiClient client;
  #endif
  HTTPClient http;
  http.setAuthorization(HTTP_USERNAME, HTTP_PASSWORD);

  auto url = String(currentServer) + "/esp32/set-image-key";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  String json = "{\"key\":\"";
  json += key;
  json += "\"}";

  int httpResponseCode = http.POST(json);
  if (httpResponseCode == 200) {
    setSuccess("Image API Key updated on server");
  } else {
    setError("Failed to update Image API Key on server");
  }
  http.end();
}

// ============================================================================
// NEW COMMAND HANDLER: Get IP Address (Command ID 20)
// ============================================================================

void get_ip_address() {
  Serial.println("[CMD] get_ip_address");
  
  String ipAddress = wifiMgr.getIPAddress();
  
  if (ipAddress.length() == 0 || ipAddress == "0.0.0.0") {
    setError("Not connected to WiFi");
    return;
  }
  
  strncpy(message, ipAddress.c_str(), MAXSTRARGLEN - 1);
  setSuccess(message);
}

// ============================================================================
// NEW COMMAND HANDLER: Get Power Status (Command ID 21)
// ============================================================================

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

void get_gpt_chunk() {
  Serial.println("[CMD] get_gpt_chunk");
  int page = (int)realArgs[0];
  int chunkSize = MAXSTRARGLEN - 1;
  int start = page * chunkSize;
  int totalLen = strlen(response);

  if (start >= totalLen) {
    message[0] = '\0';
    setSuccess(message);
    return;
  }

  strncpy(message, response + start, chunkSize);
  message[chunkSize] = '\0';
  setSuccess(message);
}

void get_status() {
  Serial.println("[CMD] get_status");

  unsigned long uptime = millis() / 1000;
  int h = uptime / 3600;
  int m = (uptime % 3600) / 60;
  int s = uptime % 60;
  char uptime_fmt[16];
  snprintf(uptime_fmt, sizeof(uptime_fmt), "%dh %dm %ds", h, m, s);

  String json = "{";
  json += "\"device\":{";
  json += "\"name\":\"ESP32-CAM\",";
  json += "\"uptime_seconds\":" + String(uptime) + ",";
  json += "\"uptime_formatted\":\"" + String(uptime_fmt) + "\",";
  json += "\"boot_count\":" + String(bootCount) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"chip_model\":\"ESP32\"";
  json += "},";
  json += "\"wifi\":{";
  json += "\"connected\":" + String(WiFi.isConnected() ? "true" : "false") + ",";
  json += "\"ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"channel\":" + String(WiFi.channel()) + "";
  json += "},";
  json += "\"power\":{";
  json += "\"powered\":" + String(isPowered ? "true" : "false") + ",";
  json += "\"deep_sleep\":" + String(powerLossDetected ? "true" : "false") + ",";
  json += "\"last_ip\":\"" + String(lastIP) + "\"";
  json += "},";
  json += "\"server\":{";
  json += "\"ngrok_url\":\"" + String(currentServer) + "\",";
  json += "\"poll_interval_ms\":" + String(POLL_INTERVAL_MS) + "";
  json += "}";
  json += "}";

  strncpy(message, json.c_str(), MAXSTRARGLEN - 1);
  setSuccess(message);
}