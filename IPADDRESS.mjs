#!/usr/bin/env node

/**
 * IPADDRESS.mjs - Get ESP32's current IP address via HTTP
 * 
 * Usage: node IPADDRESS.mjs [esp32_ip]
 * Example: node IPADDRESS.mjs 192.168.1.100
 */

import fetch from 'node-fetch';

// Get ESP32 IP from command line argument or prompt
const esp32Ip = process.argv[2] || await getEsp32Ip();

if (!esp32Ip) {
  console.error('Error: ESP32 IP address not provided and could not be detected');
  process.exit(1);
}

try {
  const response = await fetch(`http://${esp32Ip}/wifi/status`);
  const data = await response.json();

  if (!data.success) {
    console.error(`Error: ${data.message}`);
    process.exit(1);
  }

  const wifiData = JSON.parse(data.data);
  
  console.log('=== ESP32 WiFi Status ===');
  console.log(`Connected: ${wifiData.connected ? 'Yes' : 'No'}`);
  console.log(`IP Address: ${wifiData.ipAddress}`);
  console.log(`SSID: ${wifiData.ssid || 'Not connected'}`);
  console.log(`Signal Strength: ${wifiData.signalStrength} dBm`);
  console.log('==========================');
  
  // Show OTA instructions
  if (wifiData.connected && wifiData.ipAddress !== '0.0.0.0') {
    console.log('\n📡 OTA Update Instructions:');
    console.log(`👉 Open browser to: http://${wifiData.ipAddress}/`);
    console.log('👉 Upload firmware .bin file');
    console.log('👉 Device will restart automatically');
  }

} catch (error) {
  console.error(`Failed to get WiFi status: ${error.message}`);
  process.exit(1);
}

/**
 * Try to detect ESP32 IP address (placeholder - would need network scanning)
 */
async function getEsp32Ip() {
  console.log('ESP32 IP address not provided.');
  console.log('Please provide the ESP32 IP address as an argument.');
  console.log('Example: node IPADDRESS.mjs 192.168.1.100');
  return null;
}