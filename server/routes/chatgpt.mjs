import express from "express";
import openai from "openai";
import i264 from "image-to-base64";
import jimp from "jimp";
import { getKeyManager } from "../keyManager.mjs";

export async function chatgpt() {
  const routes = express.Router();
  const km = getKeyManager();

  // Helper to get fresh client with latest key
  const getGptClient = () => new openai.OpenAI({
    baseURL: "https://openrouter.ai/api/v1",
    apiKey: km.getTextKey(),
    defaultHeaders: {
      "HTTP-Referer": "https://github.com/chromalock/TI-32",
      "X-Title": "TI-32 Calculator Mod",
    },
  });

  // simply answer a question
  routes.get("/ask", async (req, res) => {
    const question = req.query.question ?? "";
    if (Array.isArray(question)) {
      res.sendStatus(400);
      return;
    }

    try {
      const result = await getGptClient().chat.completions.create({
        messages: [
          {
            role: "system",
            content:
              "You are answering questions for students. Keep responses under 100 characters and only answer using uppercase letters.",
          },
          { role: "user", content: question },
        ],
        // Using model from environment variable
        model: process.env.TEXT_AI_MODEL || "google/gemini-2.0-flash-exp:free",
      });

      res.send(result.choices[0]?.message?.content ?? "no response");
    } catch (e) {
      console.error(e);
      res.sendStatus(500);
    }
  });

  // vision AI endpoint
  routes.post("/vision", async (req, res) => {
    try {
      const { image, question } = req.body;
      if (!image) {
        return res.status(400).send("No image provided");
      }

      const visionModel = process.env.IMAGE_AI_MODEL || "google/gemini-2.0-flash-exp:free";
      const visionApiKey = km.getImageKey();

      const visionClient = new openai.OpenAI({
        baseURL: "https://openrouter.ai/api/v1",
        apiKey: visionApiKey,
        defaultHeaders: {
          "HTTP-Referer": "https://github.com/chromalock/TI-32",
          "X-Title": "TI-32 Calculator Mod",
        },
      });

      const response = await visionClient.chat.completions.create({
        model: visionModel,
        messages: [
          {
            role: "user",
            content: [
              {
                type: "text",
                text: question || "DESCRIBE WHAT YOU SEE IN THIS IMAGE. KEEP IT UNDER 100 CHARS AND USE UPPERCASE.",
              },
              {
                type: "image_url",
                image_url: {
                  url: `data:image/jpeg;base64,${image}`,
                  detail: "auto",
                },
              },
            ],
          },
        ],
        max_tokens: 200,
      });

      const aiResponse = response.choices[0]?.message?.content ?? "NO RESPONSE";
      res.status(200).send(aiResponse.toUpperCase());
    } catch (error) {
      console.error("Error in vision endpoint:", error);
      res.status(500).send("ERROR PROCESSING IMAGE");
    }
  });

  // solve a math equation from an image.
  routes.post("/solve", async (req, res) => {
    try {
      const contentType = req.headers["content-type"];
      console.log("content-type:", contentType);

      if (contentType !== "image/jpg" && contentType !== "image/jpeg") {
        res.status(400);
        res.send(`bad content-type: ${contentType}`);
        return; 
      }

      const image_data = await new Promise((resolve, reject) => {
        jimp.read(req.body, (err, value) => {
          if (err) {
            reject(err);
            return;
          }
          resolve(value);
        });
      });

      const image_path = "./to_solve.jpg";

      await image_data.writeAsync(image_path);
      const encoded_image = await i264(image_path);
      console.log("Encoded Image: ", encoded_image.length, "bytes");

      const question_number = req.query.n;

      const question = question_number
        ? `What is the answer to question ${question_number}?`
        : "What is the answer to this question?";

      console.log("prompt:", question);

      const result = await getGptClient().chat.completions.create({
        messages: [
          {
            role: "system",
            content:
              "You are a helpful math tutor, specifically designed to help with basic arithmetic, but also can answer a broad range of math questions from uploaded images. You should provide answers as succinctly as possible, and always under 100 characters. Use only uppercase letters. Be as accurate as possible.",
          },
          {
            role: "user",
            content: [
              {
                type: "text",
                text: `${question} Do not explain how you found the answer. If the question is multiple-choice, give the letter answer.`,
              },
              {
                type: "image_url",
                image_url: {
                  url: `data:image/jpeg;base64,${encoded_image}`,
                  detail: "auto", // OpenRouter often handles auto/high differently, auto is safer
                },
              },
            ],
          },
        ],
        // Use a vision-capable model. Defaulting to free Gemini if not specified.
        model: process.env.IMAGE_AI_MODEL || "google/gemini-2.0-flash-exp:free",
      });

      const aiResponse = result.choices[0]?.message?.content ?? "NO RESPONSE";
      res.send(aiResponse.toUpperCase());
    } catch (e) {
      console.error(e);
      res.sendStatus(500);
    }
  });

  return routes;
}