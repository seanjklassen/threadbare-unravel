import { defineConfig } from 'vite'
import { viteSingleFile } from 'vite-plugin-singlefile'

export default defineConfig({
  plugins: [viteSingleFile()],
  
  build: {
    // Output to dist folder
    outDir: 'dist',
    
    // Generate sourcemaps for debugging
    sourcemap: false,
    
    // Don't minify for easier debugging
    minify: false,
    
    // Inline CSS
    cssCodeSplit: false
  }
})

