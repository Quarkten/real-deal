# Remote Debugging Plan for TI32 Launcher Transfer

## Problem
The TI32 program is not appearing on the calculator after sending command 5, even though the calculator shows "Done". The transfer is failing silently somewhere in the `sendProgramVariable()` function.

## Solution
Add remote debugging that sends transfer status to the server via HTTP POST, similar to the existing `postResult()` function.

## Implementation

### 1. Add Debug Posting Function
Add a new function to post debug messages to the server:

```cpp
void postDebug(String stage, String message) {
  if (!WiFi.isConnected()) return;
  
  #ifdef SECURE
    WiFiClientSecure client;
    client.setInsecure();
  #else
    WiFiClient client;
  #endif
  HTTPClient http;
  http.setAuthorization(HTTP_USERNAME, HTTP_PASSWORD);

  auto url = String(currentServer) + "/esp32/debug";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  String json = "{\"stage\":\"" + stage + "\",\"message\":\"" + message + "\"}";
  int httpResponseCode = http.POST(json);
  http.end();
}
```

### 2. Modify sendProgramVariable() Function
Add debug calls at each stage of the transfer:

```cpp
int sendProgramVariable(const char* name, const uint8_t* program, size_t variableSize) {
  Serial.print("transferring: ");
  Serial.print(name);
  Serial.print("(");
  Serial.print(variableSize);
  Serial.println(")");
  
  postDebug("START", String("name=") + name + ",size=" + variableSize);

  int dataLength = 0;
  uint8_t msg_header[4] = { COMP83P, RTS, 13, 0 };

  uint8_t rtsdata[13] = { variableSize & 0xff, variableSize >> 8, VarTypes82::VarProgram, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int nameSize = strlen(name);
  if (nameSize == 0) {
    postDebug("ERROR", "empty name");
    return 1;
  }
  memcpy(&rtsdata[3], name, min(nameSize, 8));

  postDebug("RTS_SEND", "sending RTS");
  auto rtsVal = cbl.send(msg_header, rtsdata, 13);
  if (rtsVal) {
    postDebug("RTS_FAIL", String("code=") + rtsVal);
    Serial.print("rts return: ");
    Serial.println(rtsVal);
    return rtsVal;
  }
  postDebug("RTS_OK", "RTS accepted");

  cbl.resetLines();
  postDebug("WAIT_ACK", "waiting for ACK");
  auto ackVal = cbl.get(msg_header, NULL, &dataLength, 0);
  if (ackVal || msg_header[1] != ACK) {
    postDebug("ACK_FAIL", String("code=") + ackVal + ",header1=" + msg_header[1]);
    Serial.print("ack return: ");
    Serial.println(ackVal);
    return ackVal;
  }
  postDebug("ACK_OK", "ACK received");

  postDebug("WAIT_CTS", "waiting for CTS");
  auto ctsRet = cbl.get(msg_header, NULL, &dataLength, 0);
  if (ctsRet || msg_header[1] != CTS) {
    postDebug("CTS_FAIL", String("code=") + ctsRet + ",header1=" + msg_header[1]);
    Serial.print("cts return: ");
    Serial.println(ctsRet);
    return ctsRet;
  }
  postDebug("CTS_OK", "CTS received");

  // ... continue with similar debug calls for DATA and EOT stages
  
  postDebug("SUCCESS", String("transferred ") + name);
  return 0;
}
```

### 3. Add Server Endpoint (Optional)
If you want to see debug messages in server console, add a simple endpoint to `server/routes/esp32.mjs`:

```javascript
router.post('/debug', (req, res) => {
  console.log('[ESP32 DEBUG]', req.body);
  res.json({ received: true });
});
```

## Debug Stages to Track
1. **START** - Transfer initiated with name and size
2. **RTS_SEND** - Ready To Send packet being sent
3. **RTS_OK/RTS_FAIL** - RTS result
4. **WAIT_ACK** - Waiting for calculator ACK
5. **ACK_OK/ACK_FAIL** - ACK result
6. **WAIT_CTS** - Waiting for Clear To Send
7. **CTS_OK/CTS_FAIL** - CTS result
8. **DATA_SEND** - Sending program data
9. **DATA_OK/DATA_FAIL** - Data transfer result
10. **EOT_SEND** - End Of Transfer
11. **SUCCESS** - Transfer completed successfully

## Expected Output
When you run the launcher command, you will see messages like:
- `START: name=TI32,size=1608`
- `RTS_SEND: sending RTS`
- `RTS_OK: RTS accepted`
- etc.

If any stage fails, you'll see the failure code which will help identify the exact problem.

## Alternative: Quick Test Without Server Changes
The debug posts will still work even without the server endpoint - they'll just return 404 but the ESP32 won't care. You can watch the server access logs to see the debug messages in the URL or request body.
