// Test script for multi-modal interaction system
import fetch from 'node-fetch';

// Configuration
const ESP32_IP = '192.168.1.100'; // Replace with your ESP32's IP
const SERVER_URL = `http://${ESP32_IP}`;

// Test text input
async function testTextInput() {
  console.log('Testing text input...');
  const prompt = 'Hello, how are you?';
  const response = await fetch(`${SERVER_URL}/gpt/ask?question=${encodeURIComponent(prompt)}`);
  const result = await response.text();
  console.log('Text input result:', result);
}

// Test image capture and upload
async function testImageCapture() {
  console.log('Testing image capture...');
  // Simulate capturing an image and uploading it
  const imageData = Buffer.from('simulated-image-data', 'binary');
  const response = await fetch(`${SERVER_URL}/image/upload`, {
    method: 'POST',
    body: imageData,
    headers: {
      'Content-Type': 'image/jpg',
    },
  });
  const result = await response.text();
  console.log('Image capture result:', result);
}

// Test context continuity
async function testContextContinuity() {
  console.log('Testing context continuity...');
  // Simulate switching between text and image modes
  const textPrompt = 'What is in this image?';
  const response = await fetch(`${SERVER_URL}/gpt/ask?question=${encodeURIComponent(textPrompt)}`);
  const result = await response.text();
  console.log('Context continuity result:', result);
}

// Run all tests
async function runTests() {
  try {
    await testTextInput();
    await testImageCapture();
    await testContextContinuity();
    console.log('All tests completed successfully!');
  } catch (error) {
    console.error('Test failed:', error);
  }
}

runTests();