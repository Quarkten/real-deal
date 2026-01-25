#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "config.h"
#include "config_manager.h"

// ============================================================================
// WiFi Manager - Handles WiFi Scanning, Connecting, and Status
// ============================================================================

class WiFiManager {
private:
  ConfigManager* configMgr;
  int lastScanCount = 0;
  String lastScannedNetworks[MAX_NETWORKS];

public:
  WiFiManager(ConfigManager* cfg) : configMgr(cfg) {}

  // ========================================================================
  // WiFi Scanning Operations
  // ========================================================================

  // Scan available WiFi networks and return formatted list
  // Returns: comma-separated list of SSIDs (max MAX_NETWORKS networks)
  String scanNetworks() {
    Serial.println("[WiFiManager] Starting WiFi scan...");
    
    // Scan for networks
    int n = WiFi.scanNetworks();
    lastScanCount = min(n, MAX_NETWORKS);
    
    Serial.print("[WiFiManager] Found ");
    Serial.print(lastScanCount);
    Serial.println(" networks");

    if (lastScanCount == 0) {
      Serial.println("[WiFiManager] No networks found");
      return "";
    }

    // Build formatted list of unique SSIDs
    String networkList = "";
    int uniqueCount = 0;

    for (int i = 0; i < lastScanCount; i++) {
      String ssid = WiFi.SSID(i);
      
      // Skip hidden networks (empty SSID)
      if (ssid.length() == 0) {
        continue;
      }

      // Check if SSID already in list (avoid duplicates)
      bool isDuplicate = false;
      for (int j = 0; j < uniqueCount; j++) {
        if (lastScannedNetworks[j] == ssid) {
          isDuplicate = true;
          break;
        }
      }

      if (!isDuplicate && uniqueCount < MAX_NETWORKS) {
        lastScannedNetworks[uniqueCount] = ssid;
        
        // Append to network list
        if (networkList.length() > 0) {
          networkList += "|";
        }
        networkList += ssid;
        uniqueCount++;

        // Log signal strength
        int rssi = WiFi.RSSI(i);
        Serial.print("[WiFiManager] ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(ssid);
        Serial.print(" (RSSI: ");
        Serial.print(rssi);
        Serial.println(" dBm)");
      }
    }

    Serial.print("[WiFiManager] Returning ");
    Serial.print(uniqueCount);
    Serial.println(" unique networks");
    
    return networkList;
  }

  // Get previously scanned network list
  String getLastScannedNetworks() {
    String list = "";
    for (int i = 0; i < lastScanCount; i++) {
      if (lastScannedNetworks[i].length() > 0) {
        if (list.length() > 0) {
          list += "|";
        }
        list += lastScannedNetworks[i];
      }
    }
    return list;
  }

  // ========================================================================
  // WiFi Connection Operations
  // ========================================================================

  // Connect to WiFi network with given SSID and password
  // Returns: 0 if successful, -1 if failed
  int connectToNetwork(const char* ssid, const char* password) {
    Serial.print("[WiFiManager] Attempting to connect to: ");
    Serial.println(ssid);

    // Stop any existing connection
    if (WiFi.isConnected()) {
      WiFi.disconnect(false);
      delay(100);
    }

    // Attempt connection
    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      attempts++;

      // Timeout after WIFI_CONNECT_TIMEOUT ms
      if (millis() - startTime > WIFI_CONNECT_TIMEOUT) {
        Serial.println();
        Serial.println("[WiFiManager] Connection timeout!");
        WiFi.disconnect(false);
        return -1;
      }

      // Fail if specific error
      if (WiFi.status() == WL_CONNECT_FAILED) {
        Serial.println();
        Serial.println("[WiFiManager] Connection failed!");
        WiFi.disconnect(false);
        return -1;
      }
    }

    Serial.println();
    Serial.print("[WiFiManager] Connected! IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFiManager] Connection took ");
    Serial.print(millis() - startTime);
    Serial.println(" ms");

    configMgr->setWifiConnected(true);
    return 0;
  }

  // Try to connect using saved credentials from NVS
  // Returns: 0 if successful, -1 if failed or no credentials
  int connectUsingSavedCredentials() {
    String ssid = configMgr->getSsid();
    String pass = configMgr->getPassword();

    if (ssid.length() == 0 || pass.length() == 0) {
      Serial.println("[WiFiManager] No saved WiFi credentials");
      return -1;
    }

    Serial.println("[WiFiManager] Attempting connection with saved credentials");
    return connectToNetwork(ssid.c_str(), pass.c_str());
  }

  // Save credentials and connect
  // Returns: 0 if successful, -1 if failed
  int saveAndConnect(const char* ssid, const char* password) {
    Serial.print("[WiFiManager] Saving and connecting to: ");
    Serial.println(ssid);

    // First, try to connect
    if (connectToNetwork(ssid, password) != 0) {
      Serial.println("[WiFiManager] Connection failed, not saving credentials");
      return -1;
    }

    // If connection successful, save to NVS
    configMgr->setSsid(ssid);
    configMgr->setPassword(password);
    
    Serial.println("[WiFiManager] Credentials saved to NVS");
    return 0;
  }

  // ========================================================================
  // WiFi Status Operations
  // ========================================================================

  // Check if currently connected to WiFi
  bool isConnected() {
    return WiFi.isConnected();
  }

  // Get current WiFi status as readable string
  String getStatusString() {
    wl_status_t status = WiFi.status();
    
    switch (status) {
      case WL_CONNECTED:
        return "Connected";
      case WL_NO_SHIELD:
        return "No Shield";
      case WL_IDLE_STATUS:
        return "Idle";
      case WL_NO_SSID_AVAIL:
        return "SSID Not Available";
      case WL_SCAN_COMPLETED:
        return "Scan Completed";
      case WL_CONNECT_FAILED:
        return "Connection Failed";
      case WL_CONNECTION_LOST:
        return "Connection Lost";
      case WL_DISCONNECTED:
        return "Disconnected";
      default:
        return "Unknown";
    }
  }

  // Get current IP address
  String getIPAddress() {
    if (WiFi.isConnected()) {
      return WiFi.localIP().toString();
    }
    return "0.0.0.0";
  }

  // Get current SSID
  String getCurrentSSID() {
    if (WiFi.isConnected()) {
      return WiFi.SSID();
    }
    return "";
  }

  // Get signal strength (RSSI)
  int getSignalStrength() {
    if (WiFi.isConnected()) {
      return WiFi.RSSI();
    }
    return 0;
  }

  // ========================================================================
  // WiFi Disconnect Operations
  // ========================================================================

  // Disconnect from WiFi
  void disconnect() {
    Serial.println("[WiFiManager] Disconnecting from WiFi");
    WiFi.disconnect(false);  // false = keep WiFi module on
    configMgr->setWifiConnected(false);
  }

  // ========================================================================
  // Diagnostic Operations
  // ========================================================================

  // Print WiFi status to serial
  void printStatus() {
    Serial.println("\n=== WiFi Status ===");
    Serial.print("Connected: ");
    Serial.println(isConnected() ? "Yes" : "No");
    Serial.print("Status: ");
    Serial.println(getStatusString());
    Serial.print("SSID: ");
    Serial.println(getCurrentSSID());
    Serial.print("IP Address: ");
    Serial.println(getIPAddress());
    Serial.print("Signal Strength: ");
    Serial.print(getSignalStrength());
    Serial.println(" dBm");
    Serial.println("==================\n");
  }
};

#endif // WIFI_MANAGER_H
