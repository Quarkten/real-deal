#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <WebServer.h>
#include <Update.h>
#include "config.h"

// ============================================================================
// OTA Manager - Handles Web-Based Firmware Updates
// ============================================================================

class OTAManager {
private:
  WebServer server;
  bool isUpdating = false;
  int updateProgress = 0;

public:
  OTAManager() : server(OTA_SERVER_PORT) {}

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
    String html = R"(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 OTA Update</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 600px;
      margin: 50px auto;
      padding: 20px;
      background: #f5f5f5;
    }
    .container {
      background: white;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    h1 {
      color: #333;
      text-align: center;
    }
    .status {
      background: #e3f2fd;
      padding: 10px;
      border-radius: 4px;
      margin: 10px 0;
      border-left: 4px solid #2196F3;
    }
    .form-group {
      margin: 15px 0;
    }
    label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
      color: #555;
    }
    input[type="file"] {
      padding: 8px;
      border: 1px solid #ddd;
      border-radius: 4px;
      width: 100%;
      box-sizing: border-box;
    }
    button {
      background: #4CAF50;
      color: white;
      padding: 10px 20px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 16px;
      width: 100%;
    }
    button:hover {
      background: #45a049;
    }
    .progress-container {
      display: none;
      margin: 15px 0;
    }
    .progress-bar {
      width: 100%;
      height: 20px;
      background: #ddd;
      border-radius: 4px;
      overflow: hidden;
    }
    .progress-fill {
      height: 100%;
      background: #4CAF50;
      width: 0%;
      transition: width 0.3s;
    }
    .progress-text {
      text-align: center;
      font-size: 14px;
      color: #666;
      margin-top: 5px;
    }
    .success {
      background: #c8e6c9;
      color: #2e7d32;
      border-left-color: #4CAF50;
    }
    .error {
      background: #ffcdd2;
      color: #c62828;
      border-left-color: #f44336;
    }
    .info {
      background: #fff3e0;
      color: #e65100;
      border-left-color: #FF9800;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Firmware Update</h1>
    
    <div class="status info">
      <strong>Ready for update</strong><br>
      Select a .bin firmware file and click Upload
    </div>

    <div class="form-group">
      <label for="firmware">Firmware File (.bin):</label>
      <input type="file" id="firmware" accept=".bin" required>
    </div>

    <button onclick="uploadFirmware()">Upload Firmware</button>

    <div class="progress-container" id="progressContainer">
      <div class="progress-bar">
        <div class="progress-fill" id="progressFill"></div>
      </div>
      <div class="progress-text" id="progressText">0%</div>
    </div>

    <div id="statusDiv"></div>
  </div>

  <script>
    function uploadFirmware() {
      const fileInput = document.getElementById('firmware');
      const file = fileInput.files[0];
      
      if (!file) {
        showStatus('Please select a file', 'error');
        return;
      }

      if (!file.name.endsWith('.bin')) {
        showStatus('Please select a .bin file', 'error');
        return;
      }

      const xhr = new XMLHttpRequest();
      const formData = new FormData();
      formData.append('firmware', file);

      xhr.upload.addEventListener('progress', (e) => {
        if (e.lengthComputable) {
          const percentComplete = (e.loaded / e.total) * 100;
          updateProgress(percentComplete);
        }
      });

      xhr.addEventListener('load', () => {
        if (xhr.status === 200) {
          showStatus('Update successful! Device will restart...', 'success');
          setTimeout(() => {
            showStatus('Reconnecting...', 'info');
          }, 2000);
        } else {
          showStatus('Update failed: ' + xhr.responseText, 'error');
        }
      });

      xhr.addEventListener('error', () => {
        showStatus('Upload error', 'error');
      });

      xhr.addEventListener('abort', () => {
        showStatus('Upload aborted', 'error');
      });

      showStatus('Uploading...', 'info');
      document.getElementById('progressContainer').style.display = 'block';
      xhr.open('POST', '/update');
      xhr.send(formData);
    }

    function updateProgress(percent) {
      document.getElementById('progressFill').style.width = percent + '%';
      document.getElementById('progressText').textContent = Math.round(percent) + '%';
    }

    function showStatus(message, type) {
      const div = document.getElementById('statusDiv');
      div.innerHTML = '<div class="status ' + type + '">' + message + '</div>';
    }
  </script>
</body>
</html>
)";
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
