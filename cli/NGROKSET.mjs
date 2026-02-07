#!/usr/bin/env node

/**
 * NGROKSET.mjs - Manage Ngrok URL and display ESP32 IP address
 * 
 * Usage: node NGROKSET.mjs [esp32_ip] [new_url]
 * Example: node NGROKSET.mjs 192.168.1.100 "https://abc123.ngrok-free.app"
 */

import fetch from 'node-fetch';
import readline from 'readline';

// Get ESP32 IP from command line argument or prompt
const esp32Ip = process.argv[2] || await getEsp32Ip();

if (!esp32Ip) {
  console.error('Error: ESP32 IP address not provided and could not be detected');
  process.exit(1);
}

try {
  // Get current WiFi status and IP
  const statusResponse = await fetch(`http://${esp32Ip}/wifi/status`);
  const statusData = await statusResponse.json();

  if (!statusData.success) {
    console.error(`Error getting WiFi status: ${statusData.message}`);
    process.exit(1);
  }

  const wifiStatus = JSON.parse(statusData.data);

  // Get current Ngrok URL
  const ngrokResponse = await fetch(`http://${esp32Ip}/ngrok/url`);
  const ngrokData = await ngrokResponse.json();

  if (!ngrokData.success) {
    console.error(`Error getting Ngrok URL: ${ngrokData.message}`);
    process.exit(1);
  }

  const currentNgrok = JSON.parse(ngrokData.data).ngrokUrl;

  console.log('=== Current Configuration ===');
  console.log(`ESP32 IP: ${wifiStatus.ipAddress}`);
  console.log(`Connected: ${wifiStatus.connected ? 'Yes' : 'No'}`);
  console.log(`Current Ngrok URL: ${currentNgrok || 'Not set'}`);
  console.log('============================');

  // Check if user wants to update Ngrok URL
  const newUrl = process.argv[3];
  let shouldUpdate = false;

  if (newUrl) {
    shouldUpdate = true;
  } else {
    const answer = await promptUser('Do you want to update the Ngrok URL? (y/n): ');
    shouldUpdate = answer.toLowerCase() === 'y';
  }

  if (shouldUpdate) {
    const urlToSet = newUrl || await promptUser('Enter new Ngrok URL: ');

    if (!urlToSet) {
      console.log('No URL provided, keeping current configuration.');
    } else {
      console.log(`Updating Ngrok URL to: ${urlToSet}`);
      
      const setResponse = await fetch(`http://${esp32Ip}/ngrok/url`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `url=${encodeURIComponent(urlToSet)}`
      });

      const setData = await setResponse.json();

      if (!setData.success) {
        console.error(`Error setting Ngrok URL: ${setData.message}`);
        process.exit(1);
      }

      console.log('✅ Ngrok URL updated successfully');
    }
  }

  // Show final configuration
  const finalNgrokResponse = await fetch(`http://${esp32Ip}/ngrok/url`);
  const finalNgrokData = await finalNgrokResponse.json();
  const finalNgrok = JSON.parse(finalNgrokData.data).ngrokUrl;

  console.log('\n=== Final Configuration ===');
  console.log(`ESP32 IP: ${wifiStatus.ipAddress}`);
  console.log(`Ngrok URL: ${finalNgrok}`);
  console.log('==========================');

  if (wifiStatus.connected && wifiStatus.ipAddress !== '0.0.0.0') {
    console.log('\n📡 OTA Update Instructions:');
    console.log(`👉 Open browser to: http://${wifiStatus.ipAddress}/`);
    console.log('👉 Upload firmware .bin file');
    console.log('👉 Device will restart automatically');
  }

} catch (error) {
  console.error(`Failed to manage Ngrok URL: ${error.message}`);
  process.exit(1);
}

/**
 * Prompt user for input
 */
async function promptUser(question) {
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
  console.log('Example: node NGROKSET.mjs 192.168.1.100 "https://abc123.ngrok-free.app"');
  return null;
}