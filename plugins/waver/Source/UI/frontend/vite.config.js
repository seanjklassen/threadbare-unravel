import { defineConfig } from "vite"
import { viteSingleFile } from "vite-plugin-singlefile"
import path from "path"

export default defineConfig({
  plugins: [viteSingleFile()],
  resolve: {
    alias: {
      "@threadbare/shell": path.resolve(__dirname, "../../../../../shared/ui/shell/src"),
      "@threadbare/bridge": path.resolve(__dirname, "../../../../../shared/ui/bridge"),
    },
  },
  build: {
    outDir: "dist",
    sourcemap: false,
    minify: false,
    cssCodeSplit: false,
  },
})
