#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// Command IDs for WiFi and Configuration Management
// ============================================================================

// WiFi Management Commands (IDs 15-19)
#define CMD_SCAN_NETWORKS    15
#define CMD_CONNECT_WIFI     16
#define CMD_SAVE_WIFI        17
#define CMD_GET_NGROK        18
#define CMD_SET_NGROK        19

// ============================================================================
// NVS (Non-Volatile Storage) Configuration
// ============================================================================

#define NVS_NAMESPACE        "config"
#define NVS_WIFI_SSID        "wifi_ssid"
#define NVS_WIFI_PASS        "wifi_pass"
#define NVS_NGROK_URL        "ngrok_url"
#define NVS_WIFI_CONNECTED   "wifi_connected"
#define NVS_BOOT_COUNT       "boot_count"

// ============================================================================
// Storage Size Limits
// ============================================================================

#define MAX_SSID_LEN         32
#define MAX_PASS_LEN         64
#define MAX_NGROK_URL_LEN    256
#define MAX_NETWORKS         20

// ============================================================================
// WiFi Configuration
// ============================================================================

#define WIFI_SCAN_TIMEOUT    10000  // 10 seconds for WiFi scan
#define WIFI_CONNECT_TIMEOUT 20000  // 20 seconds to connect
#define WIFI_RECONNECT_DELAY 5000   // 5 seconds between reconnect attempts

// ============================================================================
// OTA Web Server Configuration
// ============================================================================

#define OTA_SERVER_PORT      80
#define OTA_UPDATE_PATH      "/update"
#define OTA_STATUS_PATH      "/status"

// ============================================================================
// Default Values (Fallback from secrets.h)
// ============================================================================

// These are defined in secrets.h, but we reference them here for clarity
// If NVS is corrupted or empty, these fallback values are used

#endif // CONFIG_H
