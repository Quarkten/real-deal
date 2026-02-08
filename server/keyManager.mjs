let keys = {
  textKey: "",
  imageKey: ""
};

export function initKeyManager() {
  keys.textKey = process.env.OPENROUTER_API_KEY || "";
  keys.imageKey = process.env.IMAGE_OPENROUTER_API_KEY || process.env.IMAGE_API_KEY || process.env.OPENROUTER_API_KEY || process.env.REPLICATE_API_TOKEN || "";
  console.log("[KeyManager] Initialized with keys from environment");
}

export function getKeyManager() {
  return {
    getTextKey: () => keys.textKey,
    getImageKey: () => keys.imageKey,
    setTextKey: (newKey) => {
      keys.textKey = newKey;
      console.log("[KeyManager] Text API Key updated");
    },
    setImageKey: (newKey) => {
      keys.imageKey = newKey;
      console.log("[KeyManager] Image API Key updated");
    }
  };
}
