#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Preferences.h>
#include "config.h"

// ============================================================================
// Configuration Manager - Handles NVS (Non-Volatile Storage) Operations
// ============================================================================

class ConfigManager {
private:
  Preferences prefs;
  bool initialized = false;

public:
  // Initialize the preferences namespace
  void begin() {
    if (!initialized) {
      prefs.begin(NVS_NAMESPACE, false);  // false = read/write mode
      initialized = true;
      Serial.println("[ConfigManager] Initialized");
    }
  }

  // End the preferences session
  void end() {
    if (initialized) {
      prefs.end();
      initialized = false;
    }
  }

  // ========================================================================
  // WiFi SSID Operations
  // ========================================================================

  // Save WiFi SSID to NVS
  bool setSsid(const char* ssid) {
    if (!initialized) begin();
    
    if (strlen(ssid) >= MAX_SSID_LEN) {
      Serial.println("[ConfigManager] SSID too long");
      return false;
    }
    
    prefs.putString(NVS_WIFI_SSID, ssid);
    Serial.print("[ConfigManager] Saved SSID: ");
    Serial.println(ssid);
    return true;
  }

  // Get WiFi SSID from NVS
  String getSsid() {
    if (!initialized) begin();
    String ssid = prefs.getString(NVS_WIFI_SSID, "");
    if (ssid.length() > 0) {
      Serial.print("[ConfigManager] Retrieved SSID: ");
      Serial.println(ssid);
    } else {
      Serial.println("[ConfigManager] No SSID in NVS, using fallback");
    }
    return ssid;
  }

  // ========================================================================
  // WiFi Password Operations
  // ========================================================================

  // Save WiFi password to NVS
  bool setPassword(const char* password) {
    if (!initialized) begin();
    
    if (strlen(password) >= MAX_PASS_LEN) {
      Serial.println("[ConfigManager] Password too long");
      return false;
    }
    
    prefs.putString(NVS_WIFI_PASS, password);
    Serial.println("[ConfigManager] Saved WiFi password");
    return true;
  }

  // Get WiFi password from NVS
  String getPassword() {
    if (!initialized) begin();
    String pass = prefs.getString(NVS_WIFI_PASS, "");
    if (pass.length() > 0) {
      Serial.println("[ConfigManager] Retrieved WiFi password");
    } else {
      Serial.println("[ConfigManager] No password in NVS, using fallback");
    }
    return pass;
  }

  // ========================================================================
  // Ngrok URL Operations
  // ========================================================================

  // Save Ngrok URL to NVS
  bool setNgrokUrl(const char* url) {
    if (!initialized) begin();
    
    if (strlen(url) >= MAX_NGROK_URL_LEN) {
      Serial.println("[ConfigManager] Ngrok URL too long");
      return false;
    }
    
    // Basic validation: must contain ngrok.app or similar
    if (strstr(url, "ngrok") == NULL && strstr(url, "http") == NULL) {
      Serial.println("[ConfigManager] Invalid ngrok URL format");
      return false;
    }
    
    prefs.putString(NVS_NGROK_URL, url);
    Serial.print("[ConfigManager] Saved Ngrok URL: ");
    Serial.println(url);
    return true;
  }

  // Get Ngrok URL from NVS
  String getNgrokUrl() {
    if (!initialized) begin();
    String url = prefs.getString(NVS_NGROK_URL, "");
    if (url.length() > 0) {
      Serial.print("[ConfigManager] Retrieved Ngrok URL: ");
      Serial.println(url);
    } else {
      Serial.println("[ConfigManager] No Ngrok URL in NVS");
    }
    return url;
  }

  // ========================================================================
  // WiFi Connection Status Operations
  // ========================================================================

  // Save WiFi connection status
  void setWifiConnected(bool connected) {
    if (!initialized) begin();
    prefs.putUChar(NVS_WIFI_CONNECTED, connected ? 1 : 0);
    Serial.print("[ConfigManager] WiFi connected status: ");
    Serial.println(connected ? "true" : "false");
  }

  // Get WiFi connection status
  bool getWifiConnected() {
    if (!initialized) begin();
    return prefs.getUChar(NVS_WIFI_CONNECTED, 0) == 1;
  }

  // ========================================================================
  // Boot Count Operations (for diagnostics)
  // ========================================================================

  // Increment boot counter
  uint32_t incrementBootCount() {
    if (!initialized) begin();
    uint32_t bootCount = prefs.getUInt(NVS_BOOT_COUNT, 0);
    bootCount++;
    prefs.putUInt(NVS_BOOT_COUNT, bootCount);
    Serial.print("[ConfigManager] Boot count: ");
    Serial.println(bootCount);
    return bootCount;
  }

  // Get current boot count
  uint32_t getBootCount() {
    if (!initialized) begin();
    return prefs.getUInt(NVS_BOOT_COUNT, 0);
  }

  // ========================================================================
  // Factory Reset Operations
  // ========================================================================

  // Clear all configuration from NVS
  void factoryReset() {
    if (!initialized) begin();
    prefs.clear();
    Serial.println("[ConfigManager] Factory reset completed - all config cleared");
  }

  // Clear WiFi configuration only
  void clearWifiConfig() {
    if (!initialized) begin();
    prefs.remove(NVS_WIFI_SSID);
    prefs.remove(NVS_WIFI_PASS);
    prefs.remove(NVS_WIFI_CONNECTED);
    Serial.println("[ConfigManager] WiFi configuration cleared");
  }

  // Clear Ngrok URL only
  void clearNgrokUrl() {
    if (!initialized) begin();
    prefs.remove(NVS_NGROK_URL);
    Serial.println("[ConfigManager] Ngrok URL cleared");
  }

  // ========================================================================
  // Diagnostic Operations
  // ========================================================================

  // Print all stored configuration to serial
  void printAll() {
    if (!initialized) begin();
    Serial.println("\n=== Stored Configuration ===");
    Serial.print("SSID: ");
    Serial.println(getSsid());
    Serial.print("Password: ");
    String pass = getPassword();
    if (pass.length() > 0) {
      Serial.println("[*] (hidden)");
    } else {
      Serial.println("[empty]");
    }
    Serial.print("Ngrok URL: ");
    Serial.println(getNgrokUrl());
    Serial.print("WiFi Connected: ");
    Serial.println(getWifiConnected() ? "Yes" : "No");
    Serial.print("Boot Count: ");
    Serial.println(getBootCount());
    Serial.println("============================\n");
  }

  // Check if essential config exists
  bool hasEssentialConfig() {
    String ssid = getSsid();
    String pass = getPassword();
    String ngrokUrl = getNgrokUrl();
    
    bool hasWifi = ssid.length() > 0 && pass.length() > 0;
    bool hasNgrok = ngrokUrl.length() > 0;
    
    Serial.print("[ConfigManager] Has WiFi config: ");
    Serial.println(hasWifi ? "Yes" : "No");
    Serial.print("[ConfigManager] Has Ngrok config: ");
    Serial.println(hasNgrok ? "Yes" : "No");
    
    return hasWifi && hasNgrok;
  }
};

#endif // CONFIG_MANAGER_H
