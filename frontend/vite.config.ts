import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    port: 3000,
    host: true,
    proxy: {
      '^/groups': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        secure: false
      },
      '^/chat-sessions': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        secure: false
      },
      '^/integration': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        secure: false
      },
      '^/oauth': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        secure: false
      },
      '^/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        secure: false
      }
    }
  },
  build: {
    outDir: 'dist',
    sourcemap: true
  }
}) 