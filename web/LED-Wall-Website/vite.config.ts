import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      "/get_Data": {
        target: "http://127.0.0.1:5000"
      },
      "/get_yaml_Config": {
        target: "http://127.0.0.1:5000"
      }
    }
  }
})
