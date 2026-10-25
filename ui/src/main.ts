import { mount } from 'svelte'
import './app.css'
import App from './App.svelte'
import { themeStore } from '$lib/services/theme.svelte'

// Apply the saved theme before the first paint
themeStore.init()

const app = mount(App, {
  target: document.getElementById('app')!,
})

export default app
