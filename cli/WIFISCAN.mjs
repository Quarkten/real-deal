#!/usr/bin/env node

/**
 * WIFISCAN.mjs - Scan WiFi networks and display ESP32 IP address
 * 
 * Usage: node WIFISCAN.mjs [esp32_ip]
 * Example: node WIFISCAN.mjs 192.168.1.100
 */

import fetch from 'node-fetch';

// Get ESP32 IP from command line argument or prompt
const esp32Ip = process.argv[2] || await getEsp32Ip();

if (!esp32Ip) {
  console.error('Error: ESP32 IP address not provided and could not be detected');
  process.exit(1);
}

try {
  // First get current WiFi status and IP
  const statusResponse = await fetch(`http://${esp32Ip}/wifi/status`);
  const statusData = await statusResponse.json();

  if (!statusData.success) {
    console.error(`Error getting WiFi status: ${statusData.message}`);
    process.exit(1);
  }

  const wifiStatus = JSON.parse(statusData.data);

  // Then scan for available networks
  const scanResponse = await fetch(`http://${esp32Ip}/wifi/scan`);
  const scanData = await scanResponse.json();

  if (!scanData.success) {
    console.error(`Error scanning networks: ${scanData.message}`);
    process.exit(1);
  }

  const networks = JSON.parse(scanData.data).networks;

  console.log('=== WiFi Network Scan ===');
  console.log(`Current IP: ${wifiStatus.ipAddress}`);
  console.log(`Connected: ${wifiStatus.connected ? 'Yes' : 'No'}`);
  console.log(`Current SSID: ${wifiStatus.ssid || 'Not connected'}`);
  console.log('\nAvailable Networks:');

  if (networks.length === 0) {
    console.log('  No networks found');
  } else {
    networks.forEach((network, index) => {
      console.log(`  ${index + 1}. ${network}`);
    });
  }

  console.log('\n=== OTA Information ===');
  if (wifiStatus.connected && wifiStatus.ipAddress !== '0.0.0.0') {
    console.log(`📡 OTA URL: http://${wifiStatus.ipAddress}/`);
    console.log('👉 Upload firmware .bin file to update ESP32');
  } else {
    console.log('⚠️  Connect to WiFi first to enable OTA updates');
  }
  console.log('========================');

} catch (error) {
  console.error(`Failed to scan WiFi networks: ${error.message}`);
  process.exit(1);
}

/**
 * Try to detect ESP32 IP address (placeholder - would need network scanning)
 */
async function getEsp32Ip() {
  console.log('ESP32 IP address not provided.');
  console.log('Please provide the ESP32 IP address as an argument.');
  console.log('Example: node WIFISCAN.mjs 192.168.1.100');
  return null;
}