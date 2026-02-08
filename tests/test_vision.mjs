// Test script for Vision AI endpoint
import fetch from 'node-fetch';

const SERVER_URL = process.env.SERVER_URL || 'http://localhost:8080';

async function testVisionEndpoint() {
  console.log('Testing Vision AI endpoint...');

  // Dummy base64 image (a tiny black dot)
  const dummyImage = 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=';
  const question = 'WHAT IS IN THIS IMAGE?';

  try {
    const response = await fetch(`${SERVER_URL}/gpt/vision`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': 'Basic ' + Buffer.from('admin:admin').toString('base64')
      },
      body: JSON.stringify({
        image: dummyImage,
        question: question
      })
    });

    if (response.ok) {
      const result = await response.text();
      console.log('Vision AI result:', result);
    } else {
      const errorText = await response.text();
      console.error(`Vision AI failed with status ${response.status}:`, errorText);
    }
  } catch (error) {
    console.error('Test failed with error:', error.message);
  }
}

testVisionEndpoint();
