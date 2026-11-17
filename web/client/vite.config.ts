import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'
import tailwindcss from '@tailwindcss/vite'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    vueDevTools(),
	tailwindcss(),
  ],
  build: {
	  rolldownOptions: {
		  output: {
			  codeSplitting: {
				  groups: [
					  {
						  name: 'three',
						  test: /node_modules[\\/]three/,
						  minSize: 100000,
						  maxSize: 250000
					  }
				  ]
			  }
		  }
	  }
  },
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
})
