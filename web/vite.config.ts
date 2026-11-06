import { defineConfig } from "vite";
export default defineConfig({
  build: { emptyOutDir: false },
  server: { proxy: { "/api": "http://127.0.0.1:3000" } },
});
