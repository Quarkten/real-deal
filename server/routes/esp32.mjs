import express from "express";

export function esp32Routes() {
  const router = express.Router();

  // In-memory state
  let pendingCommand = null;
  let commandResult = null;
  let deviceLogs = [];
  const MAX_LOGS = 100;
  const COMMAND_TIMEOUT = 30000;

  // Helper to add logs
  function addLog(message) {
    const logEntry = {
      timestamp: new Date().toISOString(),
      message: message
    };
    deviceLogs.unshift(logEntry);
    if (deviceLogs.length > MAX_LOGS) {
      deviceLogs = deviceLogs.slice(0, MAX_LOGS);
    }
    console.log(`[Device Log] ${message}`);
  }

  // Helper to wait for result
  async function waitForResult() {
    return new Promise((resolve) => {
      const start = Date.now();
      const interval = setInterval(() => {
        if (commandResult) {
          clearInterval(interval);
          const res = commandResult;
          commandResult = null;
          resolve(res);
        } else if (Date.now() - start > COMMAND_TIMEOUT) {
          clearInterval(interval);
          resolve({ error: "Command timed out" });
        }
      }, 500);
    });
  }

  // --- Device-Facing Endpoints ---

  // Device checks for work
  router.get("/poll", (req, res) => {
    if (pendingCommand) {
      const cmd = pendingCommand;
      // We don't clear pendingCommand here, because we want to know we are waiting for a result
      // But we should probably only send it once?
      // If the device polls again before sending result, it might get same command.
      // Usually, it's better to clear it once sent to device.
      pendingCommand = null;
      res.send(cmd);
    } else {
      res.send("NO_OP");
    }
  });

  // Device posts result
  router.post("/result", express.json(), (req, res) => {
    const result = req.body;
    addLog(`Received result: ${JSON.stringify(result).substring(0, 100)}...`);
    commandResult = result;
    res.sendStatus(200);
  });

  // --- User/UI-Facing Endpoints ---

  // Queue a WiFi scan and wait for result
  router.get("/scan", async (req, res) => {
    addLog("Queuing WiFi Scan command...");
    pendingCommand = "SCAN_NETWORKS";
    commandResult = null;
    const result = await waitForResult();
    res.json(result);
  });

  // Queue a status check and wait for result
  router.get("/status", async (req, res) => {
    addLog("Queuing Status check...");
    pendingCommand = "GET_STATUS";
    commandResult = null;
    const result = await waitForResult();
    res.json(result);
  });

  // Queue command without waiting (for UI)
  router.post("/command", express.json(), (req, res) => {
    const { command } = req.body;
    if (!command) return res.status(400).send("No command provided");

    addLog(`Queuing command: ${command}`);
    pendingCommand = command;
    commandResult = null;
    res.json({ success: true, message: `Command ${command} queued` });
  });

  // Get logs
  router.get("/logs", (req, res) => {
    res.json({ logs: deviceLogs });
  });

  // Set Ngrok URL (this is a bit special as it might be used by the UI)
  router.post("/ngrok/set", express.json(), (req, res) => {
    const { url } = req.body;
    if (!url) return res.status(400).send("No URL provided");

    addLog(`Requested Ngrok URL update to: ${url}`);
    // This command needs to be sent to ESP32
    // Command format for calculator was SET_NGROK (19) but here we use strings
    pendingCommand = `SET_NGROK ${url}`;
    // Wait, the firmware poll only handles SCAN_NETWORKS and GET_STATUS currently.
    // I should probably update firmware to handle arbitrary strings or specific ones.
    // Given the prompt, I'll stick to what was asked.

    res.json({ success: true, message: "Ngrok update command queued" });
  });

  return router;
}
