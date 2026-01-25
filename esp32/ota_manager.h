#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <WebServer.h>
#include <Update.h>
#include "config.h"
#include "wifi_manager.h"
#include "config_manager.h"

// ============================================================================
// OTA Manager - Handles Web-Based Firmware Updates and WiFi/Ngrok Control
// ============================================================================

class OTAManager {
private:
  WebServer server;
  bool isUpdating = false;
  int updateProgress = 0;
  WiFiManager* wifiMgr;
  ConfigManager* configMgr;

public:
  OTAManager(WiFiManager* wm, ConfigManager* cm) : server(OTA_SERVER_PORT), wifiMgr(wm), configMgr(cm) {}

  // Helper to send JSON responses
  void sendJsonResponse(int status, bool success, const String& message, const String& data = "{}") {
    String json = "{\"success\":";
    json += success ? "true" : "false";
    json += ",\"message\":\"";
    json += message;
    json += "\",\"data\":";
    json += data;
    json += "}";
    server.send(status, "application/json", json);
  }

  // ========================================================================
  // Setup and Configuration
  // ========================================================================

  void begin() {
    Serial.print("[OTAManager] Starting web server on port ");
    Serial.println(OTA_SERVER_PORT);

    // Setup routes
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on(OTA_UPDATE_PATH, HTTP_GET, [this]() { handleUpdateGet(); });
    server.on(OTA_UPDATE_PATH, HTTP_POST, [this]() { handleUpdatePost(); }, [this]() { handleUpload(); });
    server.on(OTA_STATUS_PATH, HTTP_GET, [this]() { handleStatus(); });

    // WiFi Control Endpoints
    server.on("/wifi/status", HTTP_GET, [this]() { handleWifiStatus(); });
    server.on("/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
    server.on("/wifi/connect", HTTP_POST, [this]() { handleWifiConnect(); });
    server.on("/wifi/save", HTTP_POST, [this]() { handleWifiSave(); });

    // Ngrok Control Endpoints
    server.on("/ngrok/url", HTTP_GET, [this]() { handleNgrokGet(); });
    server.on("/ngrok/url", HTTP_POST, [this]() { handleNgrokSet(); });

    server.begin();
    Serial.println("[OTAManager] Web server started");
  }

  void handleClient() {
    server.handleClient();
  }

  void stop() {
    server.stop();
    Serial.println("[OTAManager] Web server stopped");
  }

  // ========================================================================
  // HTTP Request Handlers
  // ========================================================================

  void handleRoot() {
    String html = "<!DOCTYPE html>\n"
                  "<html>\n"
                  "<head>\n"
                  "  <title>ESP32 OTA Update</title>\n"
                  "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                  "  <style>\n"
                  "    body {\n"
                  "      font-family: Arial, sans-serif;\n"
                  "      max-width: 600px;\n"
                  "      margin: 50px auto;\n"
                  "      padding: 20px;\n"
                  "      background: #f5f5f5;\n"
                  "    }\n"
                  "    .container {\n"
                  "      background: white;\n"
                  "      padding: 20px;\n"
                  "      border-radius: 8px;\n"
                  "      box-shadow: 0 2px 4px rgba(0,0,0,0.1);\n"
                  "    }\n"
                  "    h1 {\n"
                  "      color: #333;\n"
                  "      text-align: center;\n"
                  "    }\n"
                  "    .status {\n"
                  "      background: #e3f2fd;\n"
                  "      padding: 10px;\n"
                  "      border-radius: 4px;\n"
                  "      margin: 10px 0;\n"
                  "      border-left: 4px solid #2196F3;\n"
                  "    }\n"
                  "    .form-group {\n"
                  "      margin: 15px 0;\n"
                  "    }\n"
                  "    label {\n"
                  "      display: block;\n"
                  "      margin-bottom: 5px;\n"
                  "      font-weight: bold;\n"
                  "      color: #555;\n"
                  "    }\n"
                  "    input[type=\"file\"] {\n"
                  "      padding: 8px;\n"
                  "      border: 1px solid #ddd;\n"
                  "      border-radius: 4px;\n"
                  "      width: 100%;\n"
                  "      box-sizing: border-box;\n"
                  "    }\n"
                  "    button {\n"
                  "      background: #4CAF50;\n"
                  "      color: white;\n"
                  "      padding: 10px 20px;\n"
                  "      border: none;\n"
                  "      border-radius: 4px;\n"
                  "      cursor: pointer;\n"
                  "      font-size: 16px;\n"
                  "      width: 100%;\n"
                  "    }\n"
                  "    button:hover {\n"
                  "      background: #45a049;\n"
                  "    }\n"
                  "    .progress-container {\n"
                  "      display: none;\n"
                  "      margin: 15px 0;\n"
                  "    }\n"
                  "    .progress-bar {\n"
                  "      width: 100%;\n"
                  "      height: 20px;\n"
                  "      background: #ddd;\n"
                  "      border-radius: 4px;\n"
                  "      overflow: hidden;\n"
                  "    }\n"
                  "    .progress-fill {\n"
                  "      height: 100%;\n"
                  "      background: #4CAF50;\n"
                  "      width: 0%;\n"
                  "      transition: width 0.3s;\n"
                  "    }\n"
                  "    .progress-text {\n"
                  "      text-align: center;\n"
                  "      font-size: 14px;\n"
                  "      color: #666;\n"
                  "      margin-top: 5px;\n"
                  "    }\n"
                  "    .success {\n"
                  "      background: #c8e6c9;\n"
                  "      color: #2e7d32;\n"
                  "      border-left-color: #4CAF50;\n"
                  "    }\n"
                  "    .error {\n"
                  "      background: #ffcdd2;\n"
                  "      color: #c62828;\n"
                  "      border-left-color: #f44336;\n"
                  "    }\n"
                  "    .info {\n"
                  "      background: #fff3e0;\n"
                  "      color: #e65100;\n"
                  "      border-left-color: #FF9800;\n"
                  "    }\n"
                  "  </style>\n"
                  "</head>\n"
                  "<body>\n"
                  "  <div class=\"container\">\n"
                  "    <h1>ESP32 Firmware Update</h1>\n"
                  "    \n"
                  "    <div class=\"status info\">\n"
                  "      <strong>Ready for update</strong><br>\n"
                  "      Select a .bin firmware file and click Upload\n"
                  "    </div>\n"
                  "\n"
                  "    <div class=\"form-group\">\n"
                  "      <label for=\"firmware\">Firmware File (.bin):</label>\n"
                  "      <input type=\"file\" id=\"firmware\" accept=\".bin\" required>\n"
                  "    </div>\n"
                  "\n"
                  "    <button onclick=\"uploadFirmware()\">Upload Firmware</button>\n"
                  "\n"
                  "    <div class=\"progress-container\" id=\"progressContainer\">\n"
                  "      <div class=\"progress-bar\">\n"
                  "        <div class=\"progress-fill\" id=\"progressFill\"></div>\n"
                  "      </div>\n"
                  "      <div class=\"progress-text\" id=\"progressText\">0%</div>\n"
                  "    </div>\n"
                  "\n"
                  "    <div id=\"statusDiv\"></div>\n"
                  "  </div>\n"
                  "\n"
                  "  <script>\n"
                  "    function uploadFirmware() {\n"
                  "      const fileInput = document.getElementById('firmware');\n"
                  "      const file = fileInput.files[0];\n"
                  "      \n"
                  "      if (!file) {\n"
                  "        showStatus('Please select a file', 'error');\n"
                  "        return;\n"
                  "      }\n"
                  "\n"
                  "      if (!file.name.endsWith('.bin')) {\n"
                  "        showStatus('Please select a .bin file', 'error');\n"
                  "        return;\n"
                  "      }\n"
                  "\n"
                  "      const xhr = new XMLHttpRequest();\n"
                  "      const formData = new FormData();\n"
                  "      formData.append('firmware', file);\n"
                  "\n"
                  "      xhr.upload.addEventListener('progress', (e) => {\n"
                  "        if (e.lengthComputable) {\n"
                  "          const percentComplete = (e.loaded / e.total) * 100;\n"
                  "          updateProgress(percentComplete);\n"
                  "        }\n"
                  "      });\n"
                  "\n"
                  "      xhr.addEventListener('load', () => {\n"
                  "        if (xhr.status === 200) {\n"
                  "          showStatus('Update successful! Device will restart...', 'success');\n"
                  "          setTimeout(() => {\n"
                  "            showStatus('Reconnecting...', 'info');\n"
                  "          }, 2000);\n"
                  "        } else {\n"
                  "          showStatus('Update failed: ' + xhr.responseText, 'error');\n"
                  "        }\n"
                  "      });\n"
                  "\n"
                  "      xhr.addEventListener('error', () => {\n"
                  "        showStatus('Upload error', 'error');\n"
                  "      });\n"
                  "\n"
                  "      xhr.addEventListener('abort', () => {\n"
                  "        showStatus('Upload aborted', 'error');\n"
                  "      });\n"
                  "\n"
                  "      showStatus('Uploading...', 'info');\n"
                  "      document.getElementById('progressContainer').style.display = 'block';\n"
                  "      xhr.open('POST', '/update');\n"
                  "      xhr.send(formData);\n"
                  "    }\n"
                  "\n"
                  "    function updateProgress(percent) {\n"
                  "      document.getElementById('progressFill').style.width = percent + '%';\n"
                  "      document.getElementById('progressText').textContent = Math.round(percent) + '%';\n"
                  "    }\n"
                  "\n"
                  "    function showStatus(message, type) {\n"
                  "      const div = document.getElementById('statusDiv');\n"
                  "      div.innerHTML = '<div class=\"status ' + type + '\">' + message + '</div>';\n"
                  "    }\n"
                  "  </script>\n"
                  "</body>\n"
                  "</html>";
    server.send(200, "text/html", html);
  }

  void handleUpdateGet() {
    // Redirect to root page with update section
    server.sendHeader("Location", "/");
    server.send(302);
  }

  void handleUpdatePost() {
    if (!Update.hasError()) {
      server.send(200, "text/plain", "Update OK");
      Serial.println("[OTAManager] Update completed successfully");
      delay(1000);
      ESP.restart();
    } else {
      String errMsg = "Update failed. Error: ";
      errMsg += Update.getError();
      server.send(400, "text/plain", errMsg);
      Serial.print("[OTAManager] Update failed: ");
      Serial.println(Update.getError());
    }
  }

  void handleUpload() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
      Serial.print("[OTAManager] Update started: ");
      Serial.println(upload.filename);

      // Disable WiFi to save memory
      WiFi.mode(WIFI_STA);

      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
        Update.printError(Serial);
        server.send(400, "text/plain", "Update could not begin");
        return;
      }
      isUpdating = true;
      updateProgress = 0;
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
      if (isUpdating) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
          server.send(400, "text/plain", "Update write failed");
          return;
        }
        updateProgress += upload.currentSize;
        Serial.print(".");
      }
    }
    else if (upload.status == UPLOAD_FILE_END) {
      if (isUpdating) {
        if (Update.end(true)) {
          Serial.println();
          Serial.print("[OTAManager] Update finished. Total size: ");
          Serial.println(updateProgress);
          isUpdating = false;
        } else {
          Update.printError(Serial);
          server.send(400, "text/plain", "Update failed at end");
          return;
        }
      }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED) {
      Update.end();
      isUpdating = false;
      Serial.println("[OTAManager] Update aborted");
    }
  }

  void handleStatus() {
    String json = "{";
    json += "\"updating\":" + String(isUpdating ? "true" : "false") + ",";
    json += "\"progress\":" + String(updateProgress) + ",";
    json += "\"sketchSize\":" + String(ESP.getSketchSize()) + ",";
    json += "\"freeSpace\":" + String(ESP.getFreeSketchSpace());
    json += "}";
    server.send(200, "application/json", json);
  }

  // ========================================================================
  // WiFi Control Endpoints
  // ========================================================================

  void handleWifiStatus() {
    if (!wifiMgr) {
      sendJsonResponse(500, false, "WiFi manager not initialized");
      return;
    }

    String statusJson = "{";
    statusJson += "\"connected\":" + String(wifiMgr->isConnected() ? "true" : "false") + ",";
    statusJson += "\"ipAddress\":\"";
    statusJson += wifiMgr->getIPAddress();
    statusJson += "\",\"ssid\":\"";
    statusJson += wifiMgr->getCurrentSSID();
    statusJson += "\",\"signalStrength\":";
    statusJson += String(wifiMgr->getSignalStrength());
    statusJson += "}";

    sendJsonResponse(200, true, "WiFi status retrieved", statusJson);
  }

  void handleWifiScan() {
    if (!wifiMgr) {
      sendJsonResponse(500, false, "WiFi manager not initialized");
      return;
    }

    String networks = wifiMgr->scanNetworks();
    String networksJson = "{";
    networksJson += "\"networks\":[ ";

    if (networks.length() > 0) {
      int start = 0;
      int end = networks.indexOf('|');
      bool first = true;

      while (end != -1 || start < networks.length()) {
        if (!first) networksJson += ",";
        first = false;

        String ssid = networks.substring(start, end == -1 ? networks.length() : end);
        networksJson += "\"";
        networksJson += ssid;
        networksJson += "\"";

        if (end == -1) break;
        start = end + 1;
        end = networks.indexOf('|', start);
      }
    }

    networksJson += " ]}";
    sendJsonResponse(200, true, "WiFi scan completed", networksJson);
  }

  void handleWifiConnect() {
    if (!wifiMgr) {
      sendJsonResponse(500, false, "WiFi manager not initialized");
      return;
    }

    if (!server.hasArg("ssid") || !server.hasArg("password")) {
      sendJsonResponse(400, false, "Missing ssid or password parameter");
      return;
    }

    String ssid = server.arg("ssid");
    String password = server.arg("password");

    if (wifiMgr->connectToNetwork(ssid.c_str(), password.c_str()) != 0) {
      sendJsonResponse(500, false, "Failed to connect to WiFi network");
      return;
    }

    sendJsonResponse(200, true, "Connected to WiFi network");
  }

  void handleWifiSave() {
    if (!wifiMgr || !configMgr) {
      sendJsonResponse(500, false, "Managers not initialized");
      return;
    }

    if (!server.hasArg("ssid") || !server.hasArg("password")) {
      sendJsonResponse(400, false, "Missing ssid or password parameter");
      return;
    }

    String ssid = server.arg("ssid");
    String password = server.arg("password");

    if (!configMgr->setSsid(ssid.c_str()) || !configMgr->setPassword(password.c_str())) {
      sendJsonResponse(500, false, "Failed to save WiFi credentials");
      return;
    }

    sendJsonResponse(200, true, "WiFi credentials saved successfully");
  }

  // ========================================================================
  // Ngrok Control Endpoints
  // ========================================================================

  void handleNgrokGet() {
    if (!configMgr) {
      sendJsonResponse(500, false, "Config manager not initialized");
      return;
    }

    String ngrokUrl = configMgr->getNgrokUrl();
    String urlJson = "{";
    urlJson += "\"ngrokUrl\":\"";
    urlJson += ngrokUrl;
    urlJson += "\"}";

    sendJsonResponse(200, true, "Ngrok URL retrieved", urlJson);
  }

  void handleNgrokSet() {
    if (!configMgr) {
      sendJsonResponse(500, false, "Config manager not initialized");
      return;
    }

    if (!server.hasArg("url")) {
      sendJsonResponse(400, false, "Missing url parameter");
      return;
    }

    String url = server.arg("url");

    if (!configMgr->setNgrokUrl(url.c_str())) {
      sendJsonResponse(500, false, "Failed to save Ngrok URL");
      return;
    }

    sendJsonResponse(200, true, "Ngrok URL saved successfully");
  }

  // ========================================================================
  // Status Methods
  // ========================================================================

  bool isUpdatingFirmware() {
    return isUpdating;
  }

  int getUpdateProgress() {
    return updateProgress;
  }

  void printInfo() {
    Serial.println("\n=== OTA Manager Info ===");
    Serial.print("Server Port: ");
    Serial.println(OTA_SERVER_PORT);
    Serial.print("Update Endpoint: ");
    Serial.println(OTA_UPDATE_PATH);
    Serial.print("Status Endpoint: ");
    Serial.println(OTA_STATUS_PATH);
    Serial.print("Sketch Size: ");
    Serial.println(ESP.getSketchSize());
    Serial.print("Free Space: ");
    Serial.println(ESP.getFreeSketchSpace());
    Serial.println("========================\n");
  }
};

#endif // OTA_MANAGER_H