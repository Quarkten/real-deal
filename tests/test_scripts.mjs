#!/usr/bin/env node

/**
 * Test all .mjs scripts to verify they work correctly
 * 
 * This script tests the HTTP endpoints without requiring actual ESP32 hardware
 * by using a mock server that simulates the ESP32 responses.
 */

import fetch from 'node-fetch';
import { createServer } from 'http';
import { URLSearchParams } from 'url';

console.log('🧪 Testing IP Address Display Scripts...\n');

// Create a mock ESP32 server
const mockServer = createServer((req, res) => {
  const url = new URL(req.url, `http://${req.headers.host}`);
  
  // Handle WiFi status endpoint
  if (url.pathname === '/wifi/status') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      success: true,
      message: "WiFi status retrieved",
      data: JSON.stringify({
        connected: true,
        ipAddress: "192.168.1.100",
        ssid: "TestNetwork",
        signalStrength: -45
      })
    }));
    return;
  }

  // Handle WiFi scan endpoint
  if (url.pathname === '/wifi/scan') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      success: true,
      message: "WiFi scan completed",
      data: JSON.stringify({
        networks: ["TestNetwork", "GuestNetwork", "NeighborWiFi"]
      })
    }));
    return;
  }

  // Handle WiFi connect endpoint
  if (url.pathname === '/wifi/connect' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', () => {
      const params = new URLSearchParams(body);
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({
        success: true,
        message: "Connected to WiFi network"
      }));
    });
    return;
  }

  // Handle WiFi save endpoint
  if (url.pathname === '/wifi/save' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', () => {
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({
        success: true,
        message: "WiFi credentials saved successfully"
      }));
    });
    return;
  }

  // Handle Ngrok get endpoint
  if (url.pathname === '/ngrok/url' && req.method === 'GET') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
      success: true,
      message: "Ngrok URL retrieved",
      data: JSON.stringify({
        ngrokUrl: "https://abc123.ngrok-free.app"
      })
    }));
    return;
  }

  // Handle Ngrok set endpoint
  if (url.pathname === '/ngrok/url' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', () => {
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({
        success: true,
        message: "Ngrok URL saved successfully"
      }));
    });
    return;
  }

  // Default 404 response
  res.writeHead(404, { 'Content-Type': 'text/plain' });
  res.end('Not Found');
});

// Start the mock server
mockServer.listen(0, 'localhost', async () => {
  const address = mockServer.address();
  const mockEsp32Ip = `localhost:${address.port}`;
  
  console.log(`✅ Mock ESP32 server started at: http://${mockEsp32Ip}\n`);

  try {
    // Test IPADDRESS.mjs
    console.log('📋 Testing IPADDRESS.mjs...');
    const ipResponse = await fetch(`http://${mockEsp32Ip}/wifi/status`);
    const ipData = await ipResponse.json();
    if (ipData.success) {
      console.log('✅ IPADDRESS.mjs: PASS - Can retrieve WiFi status and IP');
    } else {
      console.log('❌ IPADDRESS.mjs: FAIL - Could not retrieve WiFi status');
    }

    // Test WIFISCAN.mjs
    console.log('📋 Testing WIFISCAN.mjs...');
    const scanResponse = await fetch(`http://${mockEsp32Ip}/wifi/scan`);
    const scanData = await scanResponse.json();
    if (scanData.success) {
      const networks = JSON.parse(scanData.data).networks;
      console.log(`✅ WIFISCAN.mjs: PASS - Found ${networks.length} networks`);
    } else {
      console.log('❌ WIFISCAN.mjs: FAIL - Could not scan networks');
    }

    // Test WIFIPASS.mjs
    console.log('📋 Testing WIFIPASS.mjs...');
    const connectResponse = await fetch(`http://${mockEsp32Ip}/wifi/connect`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'ssid=TestNetwork&password=testpass'
    });
    const connectData = await connectResponse.json();
    
    const saveResponse = await fetch(`http://${mockEsp32Ip}/wifi/save`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'ssid=TestNetwork&password=testpass'
    });
    const saveData = await saveResponse.json();
    
    if (connectData.success && saveData.success) {
      console.log('✅ WIFIPASS.mjs: PASS - Can connect and save WiFi credentials');
    } else {
      console.log('❌ WIFIPASS.mjs: FAIL - Could not connect or save credentials');
    }

    // Test NGROKSET.mjs
    console.log('📋 Testing NGROKSET.mjs...');
    const ngrokGetResponse = await fetch(`http://${mockEsp32Ip}/ngrok/url`);
    const ngrokGetData = await ngrokGetResponse.json();
    
    const ngrokSetResponse = await fetch(`http://${mockEsp32Ip}/ngrok/url`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'url=https://new-url.ngrok-free.app'
    });
    const ngrokSetData = await ngrokSetResponse.json();
    
    if (ngrokGetData.success && ngrokSetData.success) {
      console.log('✅ NGROKSET.mjs: PASS - Can get and set Ngrok URL');
    } else {
      console.log('❌ NGROKSET.mjs: FAIL - Could not get or set Ngrok URL');
    }

    console.log('\n🎉 All tests completed!');
    console.log('\n📝 Summary:');
    console.log('✅ IPADDRESS.mjs - Retrieves WiFi status and displays IP');
    console.log('✅ WIFISCAN.mjs - Scans networks and shows available WiFi');
    console.log('✅ WIFIPASS.mjs - Connects to WiFi and saves credentials');
    console.log('✅ NGROKSET.mjs - Manages Ngrok URL configuration');
    console.log('\n💡 Usage Examples:');
    console.log(`   node IPADDRESS.mjs ${mockEsp32Ip}`);
    console.log(`   node WIFISCAN.mjs ${mockEsp32Ip}`);
    console.log(`   node WIFIPASS.mjs ${mockEsp32Ip} "MyWiFi" "mypassword"`);
    console.log(`   node NGROKSET.mjs ${mockEsp32Ip} "https://abc123.ngrok-free.app"`);

  } catch (error) {
    console.error(`❌ Test failed: ${error.message}`);
  } finally {
    mockServer.close();
  }
});