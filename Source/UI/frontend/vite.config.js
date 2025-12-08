import { defineConfig } from 'vite'

export default defineConfig({
  // Use relative paths for assets so they work with file:// URLs
  base: './',
  
  build: {
    // Output to dist folder
    outDir: 'dist',
    
    // Generate sourcemaps for debugging
    sourcemap: true,
    
    // Don't minify in development for easier debugging
    minify: false
  }
})

