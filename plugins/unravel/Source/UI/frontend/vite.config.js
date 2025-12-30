import { defineConfig } from 'vite'
import { viteSingleFile } from 'vite-plugin-singlefile'
import path from 'path'

export default defineConfig({
  plugins: [viteSingleFile()],
  
  resolve: {
    alias: {
      // Resolve shared UI shell from monorepo root
      '@threadbare/shell': path.resolve(__dirname, '../../../../../shared/ui/shell/src'),
    }
  },
  
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

