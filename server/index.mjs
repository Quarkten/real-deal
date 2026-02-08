import express from "express";
import fs from "fs";
import cors from "cors";
import bodyParser from "body-parser";
import morgan from "morgan";
import dot from "dotenv";
import path from "path";
import { fileURLToPath } from "url";
import { chatgpt } from "./routes/chatgpt.mjs";
import { images } from "./routes/images.mjs";
import { chat } from "./routes/chat.mjs";
import { programs } from "./routes/programs.mjs";
import { esp32Routes } from "./routes/esp32.mjs";
import { initKeyManager } from "./keyManager.mjs";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

dot.config();
initKeyManager();

// Ensure required directories exist
const requiredDirs = ["images"];
requiredDirs.forEach(dir => {
  const fullPath = path.join(process.cwd(), dir);
  if (!fs.existsSync(fullPath)) {
    fs.mkdirSync(fullPath, { recursive: true });
    console.log(`[Setup] Created missing directory: ${fullPath}`);
  }
});

async function main() {
  const port = +(process.env.PORT ?? 8080);
  if (!port || !Number.isInteger(port)) {
    console.error("bad port");
    process.exit(1);
  }

  const app = express();
  app.use(morgan("dev"));
  app.use(cors("*"));
  app.use(express.static(path.join(__dirname, "public")));
  app.use(bodyParser.json({ limit: "10mb" }));
  app.use(
    bodyParser.raw({
      type: ["image/jpeg", "image/jpg"],
      limit: "10mb",
    })
  );
  app.use((req, res, next) => {
    console.log(req.headers.authorization);
    next();
  });

  // Programs
  app.use("/programs", programs());

  // ChatGPT
  app.use("/gpt", await chatgpt());

  // Chat
  app.use("/chats", await chat());

  // Images
  app.use("/image", images());

  // ESP32 Mailbox
  app.use("/esp32", esp32Routes());

  app.listen(port, () => {
    console.log(`listening on ${port}`);
  });
}

main();
