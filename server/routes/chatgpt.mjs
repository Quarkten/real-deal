import express from "express";
import openai from "openai";
import i264 from "image-to-base64";
import jimp from "jimp";

export async function chatgpt() {
  const routes = express.Router();

  // Configure Client for OpenRouter
  const gpt = new openai.OpenAI({
    baseURL: "https://openrouter.ai/api/v1",
    apiKey: process.env.OPENROUTER_API_KEY, // Make sure to add this to your .env file
    defaultHeaders: {
      "HTTP-Referer": "https://github.com/chromalock/TI-32", // Optional: Your site URL
      "X-Title": "TI-32 Calculator Mod", // Optional: Your site name
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
      const result = await gpt.chat.completions.create({
        messages: [
          {
            role: "system",
            content:
              "You are answering questions for students. Keep responses under 100 characters and only answer using uppercase letters.",
          },
          { role: "user", content: question },
        ],
        // Using the free model requested
        model: "kwaipilot/kat-coder-pro:free", 
      });

      res.send(result.choices[0]?.message?.content ?? "no response");
    } catch (e) {
      console.error(e);
      res.sendStatus(500);
    }
  });

  // solve a math equation from an image.
  routes.post("/solve", async (req, res) => {
    try {
      const contentType = req.headers["content-type"];
      console.log("content-type:", contentType);

      if (contentType !== "image/jpg") {
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

      const result = await gpt.chat.completions.create({
        messages: [
          {
            role: "system",
            content:
              "You are a helpful math tutor, specifically designed to help with basic arithmetic, but also can answer a broad range of math questions from uploaded images. You should provide answers as succinctly as possible, and always under 100 characters. Be as accurate as possible.",
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
        // Note: The 'kat-coder-pro' model likely does not support images.
        // We use a vision-capable model supported by OpenRouter here.
        // You can switch this to "google/gemini-2.0-flash-exp:free" for a free vision alternative.
        model: "openai/gpt-4o", 
      });

      res.send(result.choices[0]?.message?.content ?? "no response");
    } catch (e) {
      console.error(e);
      res.sendStatus(500);
    }
  });

  return routes;
}