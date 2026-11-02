import { svelte } from '@sveltejs/vite-plugin-svelte'
import { defineConfig, type Plugin } from 'vite'
import tailwindcss from "@tailwindcss/vite";
import {viteSingleFile} from "vite-plugin-singlefile";
import fs from 'node:fs'
import path from 'path'
import zlib from 'node:zlib'

// Text files the firmware serves gzipped (see HttpHandlers.cpp); images are compressed already
const COMPRESSED = /\.(html|css|js|svg)$/

/**
 * Gzips the written files in place for prebuild-ui.py to embed, once viteSingleFile has inlined the
 * scripts and styles into index.html. Runs in writeBundle, after every plugin's generateBundle:
 * vite-plugin-compression2 hooked in earlier than that on Vite 8 and compressed the scripts away
 * before they were inlined, which shipped an empty page (v1.3.0).
 */
function gzipForFirmware(): Plugin {
  return {
    name: 'gzip-for-firmware',
    apply: 'build',
    enforce: 'post',
    writeBundle(options) {
      const outDir = options.dir!
      // The board only serves index.html, so anything it links to would be missing there
      const html = fs.readFileSync(path.join(outDir, 'index.html'), 'utf8')
      const external = html.match(/<(script|link)\b[^>]*\b(src|href)="\.?\/?[^"]+\.(js|css)"/)
      if (external) throw new Error(`index.html still links to ${external[0]}; the scripts and styles must be inlined`)

      for (const file of fs.readdirSync(outDir)) {
        if (!COMPRESSED.test(file)) continue
        const filePath = path.join(outDir, file)
        fs.writeFileSync(`${filePath}.gz`, zlib.gzipSync(fs.readFileSync(filePath), { level: 9 }))
        fs.rmSync(filePath)
      }
    }
  }
}

// https://vite.dev/config/
export default defineConfig({
  plugins: [
      svelte(),
      tailwindcss(),
      viteSingleFile(),
      gzipForFirmware()
  ],
  resolve: {
    alias: {
      '$lib': path.resolve(import.meta.dirname, './src/lib')
    }
  },
  build: {
    // 1. Force all assets into the root folder (no /assets/ subfolder)
    assetsDir: './',
    // 2. Clear the folder before building
    emptyOutDir: true,
    // 3. Optional: Minify more aggressively for ESP32
    target: 'esnext',
    // Embed bigger assets directly into the output HTML
    assetsInlineLimit: 4096000
  },
    server: {
        proxy: {
            '/api': 'http://matrixcontroller.local/', // Your ESP32's actual IP
            '/ws': 'ws://matrixcontroller.local/' // Your ESP32's actual IP
        }
    }
})
