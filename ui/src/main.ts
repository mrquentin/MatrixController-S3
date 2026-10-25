import { mount } from 'svelte'
import './app.css'
import App from './App.svelte'
import { themeStore } from '$lib/services/theme.svelte'

// Apply the saved theme before the first paint
themeStore.init()

const app = mount(App, {
  target: document.getElementById('app')!,
})

// Lets Chrome on Android offer "Install app". Service workers need HTTPS, so this only happens when the
// controller is reached through an HTTPS reverse proxy, not at http://matrixcontroller.local
if (import.meta.env.PROD && window.isSecureContext && 'serviceWorker' in navigator) {
  navigator.serviceWorker.register('/sw.js').catch(() => {})
}

export default app
