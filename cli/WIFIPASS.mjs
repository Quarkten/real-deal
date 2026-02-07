#!/usr/bin/env node

/**
 * WIFIPASS.mjs - Connect ESP32 to WiFi network and display IP address
 * 
 * Usage: node WIFIPASS.mjs [esp32_ip] [ssid] [password]
 * Example: node WIFIPASS.mjs 192.168.1.100 "MyWiFi" "mypassword"
 */

import fetch from 'node-fetch';
import readline from 'readline';

// Get ESP32 IP from command line argument or prompt
const esp32Ip = process.argv[2] || await getEsp32Ip();

if (!esp32Ip) {
  console.error('Error: ESP32 IP address not provided and could not be detected');
  process.exit(1);
}

// Get SSID and password from command line or prompt
const ssid = process.argv[3] || await promptUser('Enter WiFi SSID: ');
const password = process.argv[4] || await promptUser('Enter WiFi password: ', true);

if (!ssid || !password) {
  console.error('Error: SSID and password are required');
  process.exit(1);
}

try {
  console.log(`Connecting to WiFi network: ${ssid}`);
  
  // Connect to WiFi
  const connectResponse = await fetch(`http://${esp32Ip}/wifi/connect`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
    },
    body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
  });

  const connectData = await connectResponse.json();

  if (!connectData.success) {
    console.error(`Error connecting to WiFi: ${connectData.message}`);
    process.exit(1);
  }

  console.log('✅ Connected to WiFi network');

  // Save credentials
  console.log('Saving WiFi credentials...');
  const saveResponse = await fetch(`http://${esp32Ip}/wifi/save`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
    },
    body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
  });

  const saveData = await saveResponse.json();

  if (!saveData.success) {
    console.error(`Error saving credentials: ${saveData.message}`);
    process.exit(1);
  }

  console.log('✅ WiFi credentials saved');

  // Get current status with new IP
  const statusResponse = await fetch(`http://${esp32Ip}/wifi/status`);
  const statusData = await statusResponse.json();

  if (!statusData.success) {
    console.error(`Error getting WiFi status: ${statusData.message}`);
    process.exit(1);
  }

  const wifiStatus = JSON.parse(statusData.data);

  console.log('\n=== WiFi Connection Successful ===');
  console.log(`SSID: ${wifiStatus.ssid}`);
  console.log(`IP Address: ${wifiStatus.ipAddress}`);
  console.log(`Signal Strength: ${wifiStatus.signalStrength} dBm`);
  console.log('==================================');

  console.log('\n📡 OTA Update Instructions:');
  console.log(`👉 Open browser to: http://${wifiStatus.ipAddress}/`);
  console.log('👉 Upload firmware .bin file');
  console.log('👉 Device will restart automatically');

} catch (error) {
  console.error(`Failed to connect to WiFi: ${error.message}`);
  process.exit(1);
}

/**
 * Prompt user for input
 */
async function promptUser(question, isPassword = false) {
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
  });

  return new Promise((resolve) => {
    rl.question(question, (answer) => {
      rl.close();
      resolve(answer);
    });
  });
}

/**
 * Try to detect ESP32 IP address (placeholder - would need network scanning)
 */
async function getEsp32Ip() {
  console.log('ESP32 IP address not provided.');
  console.log('Please provide the ESP32 IP address as an argument.');
  console.log('Example: node WIFIPASS.mjs 192.168.1.100 "MyWiFi" "mypassword"');
  return null;
}